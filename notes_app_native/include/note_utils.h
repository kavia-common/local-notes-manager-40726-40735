#ifndef NOTE_UTILS_H
#define NOTE_UTILS_H

#include <QString>
#include <QStringList>
#include <QDateTime>
#include <QList>

/*
 PUBLIC_INTERFACE
 Simple domain model and utilities header for use in non-GUI contexts (e.g., unit tests).
 This header intentionally avoids including Qt Widgets or any GUI classes so it can be built
 in headless CI environments where Qt Widgets may be unavailable.
 
 IMPORTANT: Do not include QApplication, QWidget, QMainWindow, or any Qt GUI headers from here.
*/

// PUBLIC_INTERFACE
struct Note {
    /** Domain model for a Note entity used by NoteUtils. */
    QString id;
    QString title;
    QString body;
    QStringList tags;
    QDateTime createdAt;
    QDateTime updatedAt;
};

// PUBLIC_INTERFACE
namespace NoteUtils {
    /** Utility functions for sorting and filtering. Unit-testable and GUI-free. */
    enum class SortBy { UpdatedAtDesc, TitleAsc };

    // PUBLIC_INTERFACE
    QList<Note> filterAndSort(const QList<Note>& notes,
                              const QString& query,
                              const QString& tagFilter,
                              SortBy sortBy);
}

#endif // NOTE_UTILS_H
