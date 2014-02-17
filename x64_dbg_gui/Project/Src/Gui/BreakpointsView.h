#ifndef BREAKPOINTSVIEW_H
#define BREAKPOINTSVIEW_H

#include <QtGui>
#include "StdTable.h"
#include "Bridge.h"
#include "Breakpoints.h"
#include <QDebug>
#include <QSplitter>
#include <QMenu>
#include <QVBoxLayout>

class BreakpointsView : public QWidget
{
    Q_OBJECT
public:
    explicit BreakpointsView(QWidget *parent = 0);
    void setupHardBPRightClickContextMenu();
    void setupSoftBPRightClickContextMenu();
    void setupMemBPRightClickContextMenu();

signals:
    
public slots:
    void reloadData();

    // Hardware
    void hardwareBPContextMenuSlot(const QPoint &pos);
    void removeHardBPActionSlot();
    void removeAllHardBPActionSlot();
    void enableDisableHardBPActionSlot();

    // Software
    void softwareBPContextMenuSlot(const QPoint & pos);
    void removeSoftBPActionSlot();
    void removeAllSoftBPActionSlot();
    void enableDisableSoftBPActionSlot();

    // Memory
    void memoryBPContextMenuSlot(const QPoint & pos);
    void removeMemBPActionSlot();
    void removeAllMemBPActionSlot();
    void enableDisableMemBPActionSlot();

private:
    QVBoxLayout* mVertLayout;
    QSplitter* mSplitter ;
    StdTable* mHardBPTable;
    StdTable* mSoftBPTable;
    StdTable* mMemBPTable;

    // Hardware BP Context Menu
    QAction* mHardBPRemoveAction;
    QAction* mHardBPRemoveAllAction;
    QAction* mHardBPEnableDisableAction;

    // Software BP Context Menu
    QAction* mSoftBPRemoveAction;
    QAction* mSoftBPRemoveAllAction;
    QAction* mSoftBPEnableDisableAction;
    
    // Memory BP Context Menu
    QAction* mMemBPRemoveAction;
    QAction* mMemBPRemoveAllAction;
    QAction* mMemBPEnableDisableAction;
};

#endif // BREAKPOINTSVIEW_H
