#include "mainApp.h"
#include <QApplication>
#include <QUuid>
#include <QTimer>

// ---------------- NotesRepository ----------------

static Note noteFromJson(const QJsonObject& o) {
    Note n;
    n.id = o.value("id").toString();
    n.title = o.value("title").toString();
    n.body = o.value("body").toString();
    const auto tagsArray = o.value("tags").toArray();
    for (const auto& t : tagsArray) n.tags.append(t.toString());
    n.createdAt = QDateTime::fromString(o.value("createdAt").toString(), Qt::ISODate);
    n.updatedAt = QDateTime::fromString(o.value("updatedAt").toString(), Qt::ISODate);
    return n;
}

static QJsonObject jsonFromNote(const Note& n) {
    QJsonObject o;
    o.insert("id", n.id);
    o.insert("title", n.title);
    o.insert("body", n.body);
    QJsonArray arr;
    for (const auto& t : n.tags) arr.append(t);
    o.insert("tags", arr);
    o.insert("createdAt", n.createdAt.toString(Qt::ISODate));
    o.insert("updatedAt", n.updatedAt.toString(Qt::ISODate));
    return o;
}

NotesRepository::NotesRepository(QObject* parent)
    : QObject(parent)
{
    auto dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (dir.isEmpty()) {
        dir = QDir::homePath() + "/.notes_app_native";
    }
    QDir().mkpath(dir);
    storagePath = dir + "/notes.json";
}

void NotesRepository::ensureLoaded() const {
    if (!cache.isEmpty()) return;
    QFile f(storagePath);
    if (!f.exists()) {
        cache.clear();
        return;
    }
    if (f.open(QIODevice::ReadOnly)) {
        const auto data = f.readAll();
        f.close();
        QJsonDocument doc = QJsonDocument::fromJson(data);
        cache.clear();
        if (doc.isArray()) {
            for (const auto& v : doc.array()) {
                if (v.isObject()) cache.append(noteFromJson(v.toObject()));
            }
        }
    }
}

void NotesRepository::saveAll() const {
    QJsonArray arr;
    for (const auto& n : cache) arr.append(jsonFromNote(n));
    QJsonDocument doc(arr);
    QFile f(storagePath);
    if (f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        f.write(doc.toJson());
        f.close();
    }
}

QList<Note> NotesRepository::getAll() const {
    ensureLoaded();
    return cache;
}

Note NotesRepository::getById(const QString& id) const {
    ensureLoaded();
    for (const auto& n : cache) if (n.id == id) return n;
    return {};
}

void NotesRepository::upsert(const Note& note) {
    ensureLoaded();
    bool found = false;
    for (int i = 0; i < cache.size(); ++i) {
        if (cache[i].id == note.id) {
            cache[i] = note;
            found = true;
            break;
        }
    }
    if (!found) cache.append(note);
    saveAll();
    emit repositoryChanged();
}

void NotesRepository::remove(const QString& id) {
    ensureLoaded();
    for (int i = cache.size() - 1; i >= 0; --i) {
        if (cache[i].id == id) cache.removeAt(i);
    }
    saveAll();
    emit repositoryChanged();
}

void NotesRepository::removeBulk(const QStringList& ids) {
    ensureLoaded();
    QSet<QString> set = QSet<QString>(ids.begin(), ids.end());
    for (int i = cache.size() - 1; i >= 0; --i) {
        if (set.contains(cache[i].id)) cache.removeAt(i);
    }
    saveAll();
    emit repositoryChanged();
}

// ---------------- NoteUtils ----------------

QList<Note> NoteUtils::filterAndSort(const QList<Note>& notes,
                                     const QString& query,
                                     const QString& tagFilter,
                                     NoteUtils::SortBy sortBy) {
    QList<Note> out;
    const auto q = query.trimmed().toLower();
    const bool useTag = !tagFilter.isEmpty() && tagFilter != "All";
    for (const auto& n : notes) {
        bool matchesQuery = q.isEmpty()
            || n.title.toLower().contains(q)
            || n.body.toLower().contains(q);
        bool matchesTag = !useTag || n.tags.contains(tagFilter);
        if (matchesQuery && matchesTag) out.append(n);
    }
    std::sort(out.begin(), out.end(), [sortBy](const Note& a, const Note& b){
        if (sortBy == NoteUtils::SortBy::UpdatedAtDesc) {
            return a.updatedAt > b.updatedAt;
        } else {
            return a.title.toLower() < b.title.toLower();
        }
    });
    return out;
}

// ---------------- TagEditDialog ----------------

TagEditDialog::TagEditDialog(const QStringList& initial, QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle("Edit Tags");
    auto* layout = new QVBoxLayout(this);
    input = new QLineEdit(this);
    input->setPlaceholderText("Comma-separated tags (e.g., work,ideas,personal)");
    input->setText(initial.join(","));
    helper = new QLabel("Tip: tags are case-sensitive. Use commas to separate.", this);
    auto* buttonsLayout = new QHBoxLayout();
    auto* okBtn = new QPushButton("OK", this);
    auto* cancelBtn = new QPushButton("Cancel", this);
    buttonsLayout->addStretch();
    buttonsLayout->addWidget(okBtn);
    buttonsLayout->addWidget(cancelBtn);

    layout->addWidget(input);
    layout->addWidget(helper);
    layout->addLayout(buttonsLayout);

    connect(okBtn, &QPushButton::clicked, this, &QDialog::accept);
    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);

    setMinimumWidth(420);
}

QStringList TagEditDialog::tags() const {
    auto raw = input->text().split(",", Qt::SkipEmptyParts);
    for (auto& t : raw) t = t.trimmed();
    raw.removeAll("");
    raw.removeDuplicates();
    return raw;
}

// ---------------- NotesMainWindow ----------------

NotesMainWindow::NotesMainWindow(QWidget* parent)
    : QMainWindow(parent), repo(new NotesRepository(this))
{
    setupUi();
    applyTheme();
    refreshList();
    statusBar()->showMessage("Ready");

    // Autosave draft when typing after small delay
    auto* autosaveTimer = new QTimer(this);
    autosaveTimer->setInterval(800);
    autosaveTimer->setSingleShot(true);
    connect(titleEdit, &QLineEdit::textChanged, autosaveTimer, qOverload<>(&QTimer::start));
    connect(bodyEdit, &QTextEdit::textChanged, autosaveTimer, qOverload<>(&QTimer::start));
    connect(autosaveTimer, &QTimer::timeout, this, [this](){
        if (titleEdit->text().trimmed().isEmpty() && bodyEdit->toPlainText().trimmed().isEmpty())
            return;
        // Create or update draft
        Note n = composeNoteFromEditor(currentEditingId);
        if (n.id.isEmpty()) n.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
        if (!n.createdAt.isValid()) n.createdAt = QDateTime::currentDateTimeUtc();
        n.updatedAt = QDateTime::currentDateTimeUtc();
        currentEditingId = n.id;
        repo->upsert(n);
        timestampLabel->setText(QString("Last edited: %1").arg(formatTimestamp(n.updatedAt)));
        refreshList();
    });

    connect(repo, &NotesRepository::repositoryChanged, this, &NotesMainWindow::refreshList);
}

void NotesMainWindow::setupUi() {
    setWindowTitle("Ocean Notes");
    resize(1000, 640);

    // Toolbar
    auto* tb = addToolBar("Main");
    auto* createAction = tb->addAction("New");
    auto* saveAction = tb->addAction("Save");
    auto* deleteAction = tb->addAction("Delete");
    auto* bulkDeleteAction = tb->addAction("Bulk Delete");
    connect(createAction, &QAction::triggered, this, &NotesMainWindow::onCreateNote);
    connect(saveAction, &QAction::triggered, this, &NotesMainWindow::onSaveNote);
    connect(deleteAction, &QAction::triggered, this, &NotesMainWindow::onDeleteNote);
    connect(bulkDeleteAction, &QAction::triggered, this, &NotesMainWindow::onBulkDelete);

    auto* central = new QWidget(this);
    auto* centralLayout = new QHBoxLayout(central);
    auto* splitter = new QSplitter(Qt::Horizontal, central);

    // Left - list
    auto* left = new QWidget(splitter);
    auto* leftLayout = new QVBoxLayout(left);

    searchBox = new QLineEdit(left);
    searchBox->setPlaceholderText("Search notes...");
    connect(searchBox, &QLineEdit::textChanged, this, &NotesMainWindow::onSearchTextChanged);

    auto* controlsRow = new QWidget(left);
    auto* controlsLayout = new QHBoxLayout(controlsRow);
    sortCombo = new QComboBox(controlsRow);
    sortCombo->addItem("Sort: Updated");
    sortCombo->addItem("Sort: Title");
    connect(sortCombo, qOverload<int>(&QComboBox::currentIndexChanged), this, &NotesMainWindow::onSortChanged);
    tagFilterCombo = new QComboBox(controlsRow);
    tagFilterCombo->addItem("All");
    connect(tagFilterCombo, qOverload<int>(&QComboBox::currentIndexChanged), this, &NotesMainWindow::onFilterTagChanged);
    multiSelectCheck = new QCheckBox("Multi-select", controlsRow);
    connect(multiSelectCheck, &QCheckBox::toggled, this, [this](bool on){
        notesList->setSelectionMode(on ? QAbstractItemView::ExtendedSelection : QAbstractItemView::SingleSelection);
    });
    controlsLayout->addWidget(sortCombo);
    controlsLayout->addWidget(tagFilterCombo);
    controlsLayout->addStretch();
    controlsLayout->addWidget(multiSelectCheck);

    notesList = new QListWidget(left);
    notesList->setSelectionMode(QAbstractItemView::SingleSelection);
    connect(notesList, &QListWidget::itemSelectionChanged, this, &NotesMainWindow::onListSelectionChanged);

    leftLayout->addWidget(searchBox);
    leftLayout->addWidget(controlsRow);
    leftLayout->addWidget(notesList);

    // Right - editor
    auto* right = new QWidget(splitter);
    auto* rightLayout = new QVBoxLayout(right);

    titleEdit = new QLineEdit(right);
    titleEdit->setPlaceholderText("Note title");

    bodyEdit = new QTextEdit(right);
    bodyEdit->setPlaceholderText("Write your note here...");

    timestampLabel = new QLabel("Last edited: —", right);

    auto* actionsRow = new QWidget(right);
    auto* actionsLayout = new QHBoxLayout(actionsRow);
    saveButton = new QPushButton("Save", actionsRow);
    deleteButton = new QPushButton("Delete", actionsRow);
    editTagsButton = new QPushButton("Tags", actionsRow);
    actionsLayout->addWidget(editTagsButton);
    actionsLayout->addStretch();
    actionsLayout->addWidget(saveButton);
    actionsLayout->addWidget(deleteButton);

    connect(saveButton, &QPushButton::clicked, this, &NotesMainWindow::onSaveNote);
    connect(deleteButton, &QPushButton::clicked, this, &NotesMainWindow::onDeleteNote);
    connect(editTagsButton, &QPushButton::clicked, this, &NotesMainWindow::onEditTags);

    rightLayout->addWidget(titleEdit);
    rightLayout->addWidget(bodyEdit);
    rightLayout->addWidget(timestampLabel);
    rightLayout->addWidget(actionsRow);

    splitter->addWidget(left);
    splitter->addWidget(right);
    splitter->setStretchFactor(0, 1);
    splitter->setStretchFactor(1, 2);

    centralLayout->addWidget(splitter);
    setCentralWidget(central);
}

void NotesMainWindow::applyTheme() {
    // Colors and subtle modern style
    QString primary = "#2563EB";
    QString secondary = "#F59E0B";
    QString error = "#EF4444";
    QString background = "#f9fafb";
    QString surface = "#ffffff";
    QString text = "#111827";

    QString base = QString(
        "QMainWindow { background: %1; }"
        "QWidget#Card, QListWidget, QTextEdit, QLineEdit, QComboBox {"
        "  background: %2; color: %6; border: 1px solid #e5e7eb; border-radius: 10px; "
        "}"
        "QLineEdit, QComboBox { padding: 8px; }"
        "QTextEdit { padding: 8px; }"
        "QListWidget::item { padding: 10px; }"
        "QListWidget::item:selected { background: %1; color: %2; }"
        "QToolBar { background: %2; border: 0; padding: 6px; }"
        "QToolBar QToolButton { background: %1; color: %6; border: 1px solid #e5e7eb; "
        "  border-radius: 8px; padding: 6px 10px; margin-right: 8px; }"
        "QToolBar QToolButton:hover { border-color: %3; }"
        "QPushButton { background: %1; color: %6; border: 1px solid #e5e7eb; border-radius: 8px; padding: 8px 14px; }"
        "QPushButton:hover { border-color: %3; }"
        "QPushButton#Primary { background: %2; color: %6; border: 1px solid %3; }"
        "QPushButton#Danger { background: %2; color: %6; border: 1px solid %5; }"
    ).arg(background, surface, primary, secondary, error, text);

    setStyleSheet(base);
    saveButton->setObjectName("Primary");
    deleteButton->setObjectName("Danger");
}

void NotesMainWindow::refreshList() {
    const auto all = repo->getAll();
    // Update tags model
    updateTagsModelFromNotes(all);

    // Determine sort and filter
    NoteUtils::SortBy sortBy = (sortCombo->currentIndex() == 0)
        ? NoteUtils::SortBy::UpdatedAtDesc
        : NoteUtils::SortBy::TitleAsc;
    QString query = searchBox->text();
    QString tagFilter = tagFilterCombo->currentText();

    const auto items = NoteUtils::filterAndSort(all, query, tagFilter, sortBy);

    // Rebuild list
    QString selectedId;
    if (auto* it = notesList->currentItem()) selectedId = it->data(Qt::UserRole).toString();

    notesList->clear();
    for (const auto& n : items) {
        auto* item = new QListWidgetItem(n.title.isEmpty() ? "(Untitled)" : n.title);
        item->setData(Qt::UserRole, n.id);
        item->setToolTip(QString("Updated %1\nTags: %2")
            .arg(formatTimestamp(n.updatedAt))
            .arg(n.tags.join(", ")));
        notesList->addItem(item);
        if (!selectedId.isEmpty() && n.id == selectedId) {
            item->setSelected(true);
        }
    }
}

void NotesMainWindow::updateTagsModelFromNotes(const QList<Note>& notes) {
    QSet<QString> tagset;
    for (const auto& n : notes) for (const auto& t : n.tags) tagset.insert(t);
    currentAllTags = QStringList(tagset.begin(), tagset.end());
    currentAllTags.sort(Qt::CaseInsensitive);

    const QString current = tagFilterCombo->currentText();
    tagFilterCombo->blockSignals(true);
    tagFilterCombo->clear();
    tagFilterCombo->addItem("All");
    for (const auto& t : currentAllTags) tagFilterCombo->addItem(t);
    int idx = tagFilterCombo->findText(current);
    if (idx >= 0) tagFilterCombo->setCurrentIndex(idx);
    tagFilterCombo->blockSignals(false);
}

void NotesMainWindow::onCreateNote() {
    clearEditor();
    currentEditingId.clear();
    titleEdit->setFocus();
    statusBar()->showMessage("Creating a new note…", 1500);
}

void NotesMainWindow::onSaveNote() {
    Note n = composeNoteFromEditor(currentEditingId);
    if (n.title.trimmed().isEmpty() && n.body.trimmed().isEmpty()) {
        QMessageBox::information(this, "Empty note", "Nothing to save. Add a title or body.");
        return;
    }
    if (n.id.isEmpty()) n.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    const auto now = QDateTime::currentDateTimeUtc();
    if (!n.createdAt.isValid()) n.createdAt = now;
    n.updatedAt = now;
    currentEditingId = n.id;
    repo->upsert(n);
    timestampLabel->setText(QString("Last edited: %1").arg(formatTimestamp(n.updatedAt)));
    statusBar()->showMessage("Note saved", 1500);
    refreshList();
}

void NotesMainWindow::onDeleteNote() {
    QStringList toDelete;
    if (multiSelectCheck->isChecked()) {
        for (auto* it : notesList->selectedItems()) {
            toDelete << it->data(Qt::UserRole).toString();
        }
        if (toDelete.isEmpty()) {
            QMessageBox::information(this, "No selection", "Select notes to delete.");
            return;
        }
    } else {
        if (currentEditingId.isEmpty()) {
            QMessageBox::information(this, "No note", "Nothing to delete.");
            return;
        }
        toDelete << currentEditingId;
    }

    const auto reply = QMessageBox::question(this, "Confirm delete",
        QString("Delete %1 selected note(s)? This cannot be undone.").arg(toDelete.size()));
    if (reply != QMessageBox::Yes) return;

    if (toDelete.size() == 1) repo->remove(toDelete.first());
    else repo->removeBulk(toDelete);

    clearEditor();
    currentEditingId.clear();
    refreshList();
    statusBar()->showMessage("Deleted", 1500);
}

void NotesMainWindow::onBulkDelete() {
    multiSelectCheck->setChecked(true);
    onDeleteNote();
}

void NotesMainWindow::onListSelectionChanged() {
    auto items = notesList->selectedItems();
    if (items.isEmpty()) return;
    const auto id = items.first()->data(Qt::UserRole).toString();
    const auto n = repo->getById(id);
    if (!n.id.isEmpty()) {
        loadNoteToEditor(n);
        currentEditingId = n.id;
    }
}

void NotesMainWindow::onSearchTextChanged(const QString&) {
    refreshList();
}

void NotesMainWindow::onSortChanged(int) {
    refreshList();
}

void NotesMainWindow::onFilterTagChanged(int) {
    refreshList();
}

void NotesMainWindow::onEditTags() {
    Note n = composeNoteFromEditor(currentEditingId);
    TagEditDialog dlg(n.tags, this);
    if (dlg.exec() == QDialog::Accepted) {
        n.tags = dlg.tags();
        if (n.id.isEmpty()) n.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
        const auto now = QDateTime::currentDateTimeUtc();
        if (!n.createdAt.isValid()) n.createdAt = now;
        n.updatedAt = now;
        currentEditingId = n.id;
        repo->upsert(n);
        refreshList();
    }
}

void NotesMainWindow::loadNoteToEditor(const Note& note) {
    titleEdit->blockSignals(true);
    bodyEdit->blockSignals(true);
    titleEdit->setText(note.title);
    bodyEdit->setText(note.body);
    timestampLabel->setText(QString("Last edited: %1")
        .arg(note.updatedAt.isValid() ? formatTimestamp(note.updatedAt) : "—"));
    titleEdit->blockSignals(false);
    bodyEdit->blockSignals(false);
}

void NotesMainWindow::clearEditor() {
    titleEdit->blockSignals(true);
    bodyEdit->blockSignals(true);
    titleEdit->clear();
    bodyEdit->clear();
    timestampLabel->setText("Last edited: —");
    titleEdit->blockSignals(false);
    bodyEdit->blockSignals(false);
}

Note NotesMainWindow::composeNoteFromEditor(const QString& id) const {
    Note n;
    n.id = id;
    n.title = titleEdit->text();
    n.body = bodyEdit->toPlainText();
    // Tags persisted separately via TagEditDialog; here we keep existing if available
    if (!id.isEmpty()) {
        auto existing = repo->getById(id);
        if (!existing.id.isEmpty()) n.tags = existing.tags, n.createdAt = existing.createdAt;
    }
    return n;
}

QString NotesMainWindow::formatTimestamp(const QDateTime& dt) const {
    return dt.toLocalTime().toString("yyyy-MM-dd hh:mm");
}

// ---------------- main ----------------

// PUBLIC_INTERFACE
int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    NotesMainWindow w;
    w.show();

    return app.exec();
}
