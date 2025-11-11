#include "note_utils.h"
#include <algorithm>

// Provide the same implementation used by the app, but independent of GUI headers.
QList<Note> NoteUtils::filterAndSort(const QList<Note>& notes,
                                     const QString& query,
                                     const QString& tagFilter,
                                     NoteUtils::SortBy sortBy) {
    QList<Note> out;
    const auto q = QString(query).trimmed().toLower();
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
