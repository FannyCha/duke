#include "UIApplication.h"
#include "UIRenderWindow.h"
#include "UIFileDialog.h"
#include "UIPluginDialog.h"
#include <dukeengine/Version.h>
#include <dukexcore/nodes/Commons.h>
#include <boost/filesystem.hpp>
#include <iostream>

UIApplication::UIApplication(Session::ptr s) :
    m_Session(s), //
                    m_RenderWindow(new UIRenderWindow(this)), //
                    m_FileDialog(new UIFileDialog(this)), //
                    m_PluginDialog(new UIPluginDialog(this, this, &m_Manager, qApp->applicationDirPath())), //
                    m_timerID(0) {

    // Global UI
    ui.setupUi(this);
    setCentralWidget(m_RenderWindow);

    // Preferences
    m_Preferences.loadShortcuts(this);
    m_Preferences.loadFileHistory();
    updateRecentFilesMenu();

    // Actions
    // File Actions
    connect(ui.openFileAction, SIGNAL(triggered()), this, SLOT(openFiles()));
    connect(ui.browseDirectoryAction, SIGNAL(triggered()), this, SLOT(browseDirectory()));
    connect(ui.savePlaylistAction, SIGNAL(triggered()), this, SLOT(savePlaylist()));
    connect(ui.quitAction, SIGNAL(triggered()), this, SLOT(close()));
    // Control Actions
    connect(ui.playStopAction, SIGNAL(triggered()), this, SLOT(playStop()));
    connect(ui.nextFrameAction, SIGNAL(triggered()), this, SLOT(nextFrame()));
    connect(ui.previousFrameAction, SIGNAL(triggered()), this, SLOT(previousFrame()));
    connect(ui.firstFrameAction, SIGNAL(triggered()), this, SLOT(firstFrame()));
    connect(ui.lastFrameAction, SIGNAL(triggered()), this, SLOT(lastFrame()));
    connect(ui.nextShotAction, SIGNAL(triggered()), this, SLOT(nextShot()));
    connect(ui.previousShotAction, SIGNAL(triggered()), this, SLOT(previousShot()));
    // Display Actions
    connect(ui.LINAction, SIGNAL(triggered()), this, SLOT(colorspaceLIN()));
    connect(ui.LOGAction, SIGNAL(triggered()), this, SLOT(colorspaceLOG()));
    connect(ui.SRGBAction, SIGNAL(triggered()), this, SLOT(colorspaceSRGB()));
    // Window Actions
    connect(ui.fullscreenAction, SIGNAL(triggered()), this, SLOT(fullscreen()));
    connect(ui.toggleFitModeAction, SIGNAL(triggered()), this, SLOT(toggleFitMode()));
    connect(ui.fitImageTo11Action, SIGNAL(triggered()), this, SLOT(fitToNormalSize()));
    connect(ui.fitImageToWindowWidthAction, SIGNAL(triggered()), this, SLOT(fitImageToWindowWidth()));
    connect(ui.fitImageToWindowHeightAction, SIGNAL(triggered()), this, SLOT(fitImageToWindowHeight()));
    connect(m_RenderWindow, SIGNAL(zoomChanged(double)), this, SLOT(zoom(double)));
    connect(m_RenderWindow, SIGNAL(panChanged(double,double)), this, SLOT(pan(double, double)));
    // About Actions
    connect(ui.aboutAction, SIGNAL(triggered()), this, SLOT(about()));
    connect(ui.aboutPluginsAction, SIGNAL(triggered()), this, SLOT(aboutPlugins()));

    // Registering nodes
    SceneNode::ptr sc = SceneNode::ptr(new SceneNode());
    m_Manager.addNode(sc, m_Session);
    TransportNode::ptr t = TransportNode::ptr(new TransportNode());
    m_Manager.addNode(t, m_Session);
    FitNode::ptr f = FitNode::ptr(new FitNode());
    m_Manager.addNode(f, m_Session);
    GradingNode::ptr g = GradingNode::ptr(new GradingNode());
    m_Manager.addNode(g, m_Session);
    InfoNode::ptr info = InfoNode::ptr(new InfoNode());
    m_Manager.addNode(info, m_Session);
    PlaybackNode::ptr pb = PlaybackNode::ptr(new PlaybackNode());
    m_Manager.addNode(pb, m_Session);
}

void UIApplication::showEvent(QShowEvent* event) {
    // load needed plugins
    m_PluginDialog->load(QString("plugin_dukex_imageinfo"));
    m_PluginDialog->load(QString("plugin_dukex_timeline"));
    // starting timer (to compute 'IN' msgs every N ms)
    m_timerID = QObject::startTimer(40);
    // starting session
    m_Session->start(m_RenderWindow->renderWindowID());
    event->accept();
}

void UIApplication::addObserver(QObject* _plugin, IObserver* _observer) {
    if (!_observer)
        return;
    m_Session->addObserver(_observer);
    m_RegisteredObservers.insert(_plugin, _observer);
}

QAction* UIApplication::createAction(QObject* _plugin, const QString & _menuName) {
    QAction * customaction = NULL;
    if (!_menuName.isEmpty()) {
        QList<QMenu *> menus = findChildren<QMenu *> ();
        QListIterator<QMenu *> iter(menus);
        while (iter.hasNext()) {
            QMenu *pMenu = iter.next();
            if (pMenu->objectName() != _menuName)
                continue;
            customaction = new QAction(menuBar());
            pMenu->addAction(customaction);
            break;
        }
    }

    if (customaction)
        m_LoadedUIElements.insert(_plugin, customaction);

    return customaction;
}

QMenu* UIApplication::createMenu(QObject* _plugin, const QString & _menuName) {
    QMenu * custommenu = new QMenu(menuBar());
    if (!_menuName.isEmpty()) {
        QList<QMenu *> menus = findChildren<QMenu *> ();
        QListIterator<QMenu *> iter(menus);
        while (iter.hasNext()) {
            QMenu *pMenu = iter.next();
            if (pMenu->objectName() != _menuName)
                continue;
            pMenu->addMenu(custommenu);
            break;
        }
    } else {
        menuBar()->addMenu(custommenu);
        custommenu->setParent(menuBar());
    }
    m_LoadedUIElements.insert(_plugin, custommenu);
    return custommenu;
}

QDockWidget* UIApplication::createWindow(QObject* _plugin, Qt::DockWidgetArea _area, bool floating) {
    QDockWidget * customdockwidget = new QDockWidget("undefined", this);
    customdockwidget->setContentsMargins(0, 0, 0, 0);
    connect(customdockwidget, SIGNAL(topLevelChanged(bool)), this, SLOT(topLevelChanged(bool)));
    addDockWidget(_area, customdockwidget);
    m_LoadedUIElements.insert(_plugin, customdockwidget);
    if (floating) {
        customdockwidget->setFloating(true);
        customdockwidget->move(mapToGlobal(m_RenderWindow->renderWidget()->pos()) + QPoint(40, 60));
        customdockwidget->adjustSize();
    }
    return customdockwidget;
}

void UIApplication::topLevelChanged(bool b) {
    QDockWidget *dockwidget = qobject_cast<QDockWidget *> (sender());
    if (dockwidget && b) {
        dockwidget->setWindowOpacity(0.6);
        dockwidget->move(mapToGlobal(m_RenderWindow->renderWidget()->pos()) + QPoint(40, 60));
        dockwidget->adjustSize();
    }
}

void UIApplication::closeUI(QObject* _plug) {
    // deregister observers
    QList<IObserver*> registeredobservers = m_RegisteredObservers.values(_plug);
    for (int i = 0; i < registeredobservers.size(); ++i) {
        IObserver* obs = registeredobservers.at(i);
        m_Session->removeObserver(obs);
    }
    m_RegisteredObservers.remove(_plug);

    // close & delete UI elements
    QList<QObject*> uielements = m_LoadedUIElements.values(_plug);
    for (int i = 0; i < uielements.size(); ++i) {
        if (qobject_cast<QDockWidget*> (uielements.at(i))) {
            QDockWidget* obj = qobject_cast<QDockWidget*> (uielements.at(i));
            obj->close();
            removeDockWidget(obj);
            obj->deleteLater();
        } else if (qobject_cast<QMenu*> (uielements.at(i))) {
            QMenu* obj = qobject_cast<QMenu*> (uielements.at(i));
            obj->close();
            obj->deleteLater();
        } else if (qobject_cast<QAction*> (uielements.at(i))) {
            QAction* obj = qobject_cast<QAction*> (uielements.at(i));
            obj->deleteLater();
        }
    }
    m_LoadedUIElements.remove(_plug);
}

// private
void UIApplication::closeEvent(QCloseEvent *event) {
    m_RenderWindow->close();
    m_Session->stop();
    QObject::killTimer(m_timerID);
    m_Manager.clearNodes();
    m_Preferences.saveShortcuts(this);
    m_Preferences.saveFileHistory();
    QMainWindow::closeEvent(event);
    event->accept();
}

// private
void UIApplication::timerEvent(QTimerEvent *event) {
    // retrieve in msgs
    m_Session->receiveMsg();
    event->accept();
}

// private
void UIApplication::keyPressEvent(QKeyEvent * event) {
    QMainWindow::keyPressEvent(event);
    switch (event->key()) {
        case Qt::Key_Escape:
            close();
            break;
        default:
            break;
    }
    event->accept();
}

// private
void UIApplication::dragEnterEvent(QDragEnterEvent *event) {
    event->acceptProposedAction();
}

// private
void UIApplication::dragMoveEvent(QDragMoveEvent *event) {
    event->acceptProposedAction();
}

// private
void UIApplication::dropEvent(QDropEvent *event) {
    //    const QMimeData *mimeData = event->mimeData();
    //    if (mimeData->hasUrls()) {
    //        QList < QUrl > urlList = mimeData->urls();
    //        QString text;
    //        for (int i = 0; i < urlList.size() && i < 32; ++i) {
    //            openPlaylist(urlList.at(i).path());
    //        }
    //    } else {
    //        m_statusInfo->setText(tr("Cannot display data"));
    //    }
    //    setBackgroundRole(QPalette::Dark);
    event->acceptProposedAction();
}

// private
void UIApplication::resizeCentralWidget(const QSize& resolution) {
    QSize renderRes = m_RenderWindow->size();
    if (resolution == renderRes)
        return;
    QList<int> sizes;
    QObjectList childs = children();
    // Fix size of all children widget
    for (int i = 0; i < childs.size(); ++i) {
        QDockWidget* dock = qobject_cast<QDockWidget*> (childs[i]);
        if (!dock || dock->isFloating())
            continue;
        switch (dockWidgetArea(dock)) {
            case Qt::LeftDockWidgetArea:
            case Qt::RightDockWidgetArea: // Constraint width
                sizes.append(dock->minimumWidth());
                sizes.append(dock->maximumWidth());
                dock->setFixedWidth(dock->width());
                break;
            case Qt::TopDockWidgetArea:
            case Qt::BottomDockWidgetArea: // Constraint height
                sizes.append(dock->minimumHeight());
                sizes.append(dock->maximumHeight());
                dock->setFixedHeight(dock->height());
                break;
            default:
                break;
        }
    }
    // Resize renderer to the new resolution and prevent resize event
    m_RenderWindow->blockSignals(true);
    m_RenderWindow->setFixedSize(resolution);
    m_RenderWindow->blockSignals(false);
    // resize its parent (QMainWindow)
    adjustSize();
    // Allow user to resize renderer
    m_RenderWindow->setMinimumSize(0, 0);
    m_RenderWindow->setMaximumSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX);
    // Restore min & max size of all children widget
    for (int i = 0; i < childs.size(); ++i) {
        QDockWidget* dock = qobject_cast<QDockWidget*> (childs[i]);
        if (!dock || dock->isFloating())
            continue;
        switch (dockWidgetArea(dock)) {
            case Qt::LeftDockWidgetArea:
            case Qt::RightDockWidgetArea:
                dock->setMinimumWidth(sizes.takeFirst());
                dock->setMaximumWidth(sizes.takeFirst());
                break;
            case Qt::TopDockWidgetArea:
            case Qt::BottomDockWidgetArea:
                dock->setMinimumHeight(sizes.takeFirst());
                dock->setMaximumHeight(sizes.takeFirst());
                break;
            default:
                break;
        }
    }
}

void UIApplication::updateRecentFilesMenu() {
    ui.openRecentMenu->clear();
    for (size_t i = 0; i < m_Preferences.history().size(); ++i) {
        if (m_Preferences.history(i) == "")
            continue;
        boost::filesystem::path fn(m_Preferences.history(i));
        //        QAction * act = ui.openRecentMenu->addAction(fn.leaf().c_str());
        QAction * act = ui.openRecentMenu->addAction(m_Preferences.history(i).c_str());
        act->setData(m_Preferences.history(i).c_str());
        connect(act, SIGNAL(triggered()), this, SLOT(openRecent()));
    }
}

// private slot
void UIApplication::openFiles(const QStringList & _list, const bool & browseMode, const bool & parseSequence) {
    SceneNode::ptr s = m_Manager.nodeByName<SceneNode> ("fr.mikrosimage.dukex.scene");
    if (s.get() == NULL)
        return;
    if (!_list.isEmpty()) {
        // --- QStringList to STL vector<string>
        std::vector<std::string> v;
        v.resize(_list.count());
        for (int i = 0; i < _list.count(); ++i) {
            v[i] = _list[i].toStdString();
        }
        s->openFiles(v, browseMode);
        if (v.size() == 1) { // multi selection not handled in file history
            QString history = _list[0];
            if (browseMode) {
                history.prepend("browse://");
            } else {
                if (parseSequence)
                    history.prepend("sequence://");
                else
                    history.prepend("file://");
            }
            m_Preferences.addToHistory(history.toStdString());
            updateRecentFilesMenu();
        }
    }
}

// private slot
void UIApplication::openFiles() {
    QStringList list;
    if (m_FileDialog->exec()) {
        list = m_FileDialog->selectedFiles();
        if (list.size() != 0)
            openFiles(list, false, m_FileDialog->asSequence());
    }
}

// private slot
void UIApplication::openRecent() {
    QAction *action = qobject_cast<QAction *> (sender());
    if (action) {
        QString file = action->data().toString();
        if (!file.isEmpty()) {
            QStringList filenames;
            if (file.startsWith("sequence://")) {
                file.remove(0, 11);
                filenames.append(file);
                openFiles(filenames, false, true);
            } else if (file.startsWith("browse://")) {
                file.remove(0, 9);
                filenames.append(file);
                openFiles(filenames, true, false);
            } else { // "file://"
                file.remove(0, 7);
                filenames.append(file);
                openFiles(filenames, false, false);
            }
        }
    }
}

// private slot
void UIApplication::browseDirectory() {
    QStringList list;
    if (m_FileDialog->exec()) {
        list = m_FileDialog->selectedFiles();
        if (list.size() != 0)
            openFiles(list, true, m_FileDialog->asSequence());
    }
}

// private slot
void UIApplication::savePlaylist() {
    QString file = QFileDialog::getSaveFileName(this, tr("Save Current Playlist"), QString(), tr("Duke Playlist (*.dk);;All(*)"));
    if (!file.isEmpty()) {
        if(!file.endsWith(".dk"))
            file.append(".dk");
        SceneNode::ptr scene = m_Manager.nodeByName<SceneNode> ("fr.mikrosimage.dukex.scene");
        if (scene.get() == NULL)
            return;
        scene->save(file.toStdString());
    }
}

// private slot
void UIApplication::playStop() {
    TransportNode::ptr t = m_Manager.nodeByName<TransportNode> ("fr.mikrosimage.dukex.transport");
    if (t.get() == NULL)
        return;
    if (m_Session->descriptor().isPlaying())
        t->stop();
    else
        t->play();
}

// private slot
void UIApplication::previousFrame() {
    TransportNode::ptr t = m_Manager.nodeByName<TransportNode> ("fr.mikrosimage.dukex.transport");
    if (t.get() == NULL)
        return;
    t->previousFrame();
}

// private slot
void UIApplication::nextFrame() {
    TransportNode::ptr t = m_Manager.nodeByName<TransportNode> ("fr.mikrosimage.dukex.transport");
    if (t.get() == NULL)
        return;
    t->nextFrame();
}

// private slot
void UIApplication::firstFrame() {
    TransportNode::ptr t = m_Manager.nodeByName<TransportNode> ("fr.mikrosimage.dukex.transport");
    if (t.get() == NULL)
        return;
    t->firstFrame();
}

// private slot
void UIApplication::lastFrame() {
    TransportNode::ptr t = m_Manager.nodeByName<TransportNode> ("fr.mikrosimage.dukex.transport");
    if (t.get() == NULL)
        return;
    t->lastFrame();
}

// private slot
void UIApplication::previousShot() {
    TransportNode::ptr t = m_Manager.nodeByName<TransportNode> ("fr.mikrosimage.dukex.transport");
    if (t.get() == NULL)
        return;
    t->previousShot();
}

// private slot
void UIApplication::nextShot() {
    TransportNode::ptr t = m_Manager.nodeByName<TransportNode> ("fr.mikrosimage.dukex.transport");
    if (t.get() == NULL)
        return;
    t->nextShot();
}

// private slot
void UIApplication::colorspaceLIN() {
    GradingNode::ptr g = m_Manager.nodeByName<GradingNode> ("fr.mikrosimage.dukex.grading");
    if (g.get() == NULL)
        return;
    g->setColorspace(::duke::playlist::Display::LIN);
}

// private slot
void UIApplication::colorspaceLOG() {
    GradingNode::ptr g = m_Manager.nodeByName<GradingNode> ("fr.mikrosimage.dukex.grading");
    if (g.get() == NULL)
        return;
    g->setColorspace(::duke::playlist::Display::LOG);
}

// private slot
void UIApplication::colorspaceSRGB() {
    GradingNode::ptr g = m_Manager.nodeByName<GradingNode> ("fr.mikrosimage.dukex.grading");
    if (g.get() == NULL)
        return;
    g->setColorspace(::duke::playlist::Display::SRGB);
}

// private slot
void UIApplication::fullscreen() {
    if (m_RenderWindow->isFullScreen()) {
        m_RenderWindow->showNormal();
    } else {
        m_RenderWindow->showFullScreen();
    }
}

// private slot
void UIApplication::toggleFitMode() {
    FitNode::ptr f = m_Manager.nodeByName<FitNode> ("fr.mikrosimage.dukex.fit");
    if (f.get() == NULL)
        return;
    f->toggle();
}

// private slot
void UIApplication::fitToNormalSize() {
    FitNode::ptr f = m_Manager.nodeByName<FitNode> ("fr.mikrosimage.dukex.fit");
    if (f.get() == NULL)
        return;
    f->fitToNormalSize();
}

// private slot
void UIApplication::fitImageToWindowWidth() {
    FitNode::ptr f = m_Manager.nodeByName<FitNode> ("fr.mikrosimage.dukex.fit");
    if (f.get() == NULL)
        return;
    f->fitImageToWindowWidth();
}

// private slot
void UIApplication::fitImageToWindowHeight() {
    FitNode::ptr f = m_Manager.nodeByName<FitNode> ("fr.mikrosimage.dukex.fit");
    if (f.get() == NULL)
        return;
    f->fitImageToWindowHeight();
}

// private slot
void UIApplication::zoom(double z) {
    GradingNode::ptr g = m_Manager.nodeByName<GradingNode> ("fr.mikrosimage.dukex.grading");
    if (g.get() == NULL)
        return;
    g->setZoom(z);
}

// private slot
void UIApplication::pan(double x, double y) {
    GradingNode::ptr g = m_Manager.nodeByName<GradingNode> ("fr.mikrosimage.dukex.grading");
    if (g.get() == NULL)
        return;
    g->setPan(x, y);
}

// private slot
void UIApplication::about() {
    QString msg(getVersion("DukeX").c_str());
    QMessageBox::about(this, tr("About DukeX"), msg);
}

// private slot
void UIApplication::aboutPlugins() {
    m_PluginDialog->exec();
}

