#include <QtTest/QtTest>
#include "note_utils.h"

 // This test must remain GUI-free. Do NOT include mainApp.h or any Qt Widgets headers here.
 // Compile-time guard to ensure no accidental GUI inclusion.
 #ifdef QT_WIDGETS_LIB
 #error "Qt Widgets should not be linked or included in NoteUtilsTest."
 #endif

 // Implementation note:
 // - Linked libraries are limited to Qt6::Core and Qt6::Test only (see CMakeLists).
 // - Do not instantiate QApplication or QWidget-derived classes here.
 // - Keep this test runnable in headless CI environments.

class NoteUtilsTestCase : public QObject {
    Q_OBJECT
private slots:
    void test_filter_and_sort() {
        QList<Note> notes;
        Note a; a.id="1"; a.title="Alpha"; a.body="First body"; a.tags={"work"}; a.createdAt=QDateTime::currentDateTimeUtc().addDays(-2); a.updatedAt=a.createdAt.addSecs(10);
        Note b; b.id="2"; b.title="beta"; b.body="Second body"; b.tags={"ideas"}; b.createdAt=QDateTime::currentDateTimeUtc().addDays(-1); b.updatedAt=b.createdAt.addSecs(20);
        Note c; c.id="3"; c.title="Gamma"; c.body="Third body"; c.tags={"work", "ideas"}; c.createdAt=QDateTime::currentDateTimeUtc(); c.updatedAt=c.createdAt.addSecs(30);
        notes << a << b << c;

        // Search query
        auto out1 = NoteUtils::filterAndSort(notes, "body", "All", NoteUtils::SortBy::UpdatedAtDesc);
        QCOMPARE(out1.size(), 3);

        // Tag filter
        auto out2 = NoteUtils::filterAndSort(notes, "", "work", NoteUtils::SortBy::UpdatedAtDesc);
        QCOMPARE(out2.size(), 2);

        // Sort by title asc (case-insensitive)
        auto out3 = NoteUtils::filterAndSort(notes, "", "All", NoteUtils::SortBy::TitleAsc);
        QVERIFY(out3.size() == 3);
        QCOMPARE(out3[0].title, QString("Alpha"));
        QCOMPARE(out3[1].title.toLower(), QString("beta"));
        QCOMPARE(out3[2].title, QString("Gamma"));
    }
};

QTEST_MAIN(NoteUtilsTestCase)
#include "note_utils_test.moc"
