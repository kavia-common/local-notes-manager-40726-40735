#ifndef MAIN_APP_H
#define MAIN_APP_H

#include <QWidget>
#include <QMainWindow>
#include <QListWidget>
#include <QLineEdit>
#include <QTextEdit>
#include <QPushButton>
#include <QComboBox>
#include <QCheckBox>
#include <QLabel>
#include <QDateTime>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QFile>
#include <QDir>
#include <QStandardPaths>
#include <QMessageBox>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QSplitter>
#include <QToolBar>
#include <QAction>
#include <QStatusBar>
#include <QDialog>
#include <QFormLayout>
#include <QListWidgetItem>

/*
 Ocean Professional Theme (Blue & amber accents):
 - primary: #2563EB
 - secondary/success: #F59E0B
 - error: #EF4444
 - background: #f9fafb
 - surface: #ffffff
 - text: #111827
 Rounded corners, subtle shadows, smooth transitions.
*/

// PUBLIC_INTERFACE
struct Note {
    /** Domain model for a Note entity. */
    QString id;
    QString title;
    QString body;
    QStringList tags;
    QDateTime createdAt;
    QDateTime updatedAt;
};

// PUBLIC_INTERFACE
class NotesRepository : public QObject {
    Q_OBJECT
public:
    /** Local JSON file repository with simple read/write operations for notes. */
    explicit NotesRepository(QObject* parent = nullptr);

    // PUBLIC_INTERFACE
    QList<Note> getAll() const;
    // PUBLIC_INTERFACE
    Note getById(const QString& id) const;
    // PUBLIC_INTERFACE
    void upsert(const Note& note);
    // PUBLIC_INTERFACE
    void remove(const QString& id);
    // PUBLIC_INTERFACE
    void removeBulk(const QStringList& ids);

signals:
    void repositoryChanged();

private:
    QString storagePath;
    mutable QList<Note> cache;

    void ensureLoaded() const;
    void saveAll() const;
};

// PUBLIC_INTERFACE
namespace NoteUtils {
    /** Utility functions for sorting and filtering. Unit-testable. */
    enum class SortBy { UpdatedAtDesc, TitleAsc };

    // PUBLIC_INTERFACE
    QList<Note> filterAndSort(const QList<Note>& notes,
                              const QString& query,
                              const QString& tagFilter,
                              SortBy sortBy);
}

// Dialog for tag editing
class TagEditDialog : public QDialog {
    Q_OBJECT
public:
    explicit TagEditDialog(const QStringList& initial, QWidget* parent = nullptr);
    QStringList tags() const;
private:
    QLineEdit* input;
    QLabel* helper;
};

// Main Window
class NotesMainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit NotesMainWindow(QWidget* parent = nullptr);

private slots:
    void onCreateNote();
    void onSaveNote();
    void onDeleteNote();
    void onBulkDelete();
    void onListSelectionChanged();
    void onSearchTextChanged(const QString& text);
    void onSortChanged(int index);
    void onFilterTagChanged(int index);
    void onEditTags();

private:
    NotesRepository* repo;

    // Left panel - list and controls
    QLineEdit* searchBox;
    QComboBox* sortCombo;
    QComboBox* tagFilterCombo;
    QListWidget* notesList;
    QCheckBox* multiSelectCheck;

    // Right panel - editor
    QLineEdit* titleEdit;
    QTextEdit* bodyEdit;
    QLabel* timestampLabel;
    QPushButton* saveButton;
    QPushButton* deleteButton;
    QPushButton* editTagsButton;

    // State
    QString currentEditingId;
    QStringList currentAllTags;

    void setupUi();
    void applyTheme();
    void refreshList();
    void loadNoteToEditor(const Note& note);
    void clearEditor();
    void updateTagsModelFromNotes(const QList<Note>& notes);
    Note composeNoteFromEditor(const QString& id = QString()) const;
    QString formatTimestamp(const QDateTime& dt) const;
};

// PUBLIC_INTERFACE
int main(int argc, char *argv[]);

#endif // MAIN_APP_H