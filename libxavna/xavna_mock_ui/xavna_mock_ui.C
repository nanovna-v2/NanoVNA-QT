#include "xavna_mock_ui.H"
#include "xavna_mock_ui_dialog.H"
#include <QApplication>
#include <pthread.h>
#include <unistd.h>

QApplication* app;
pthread_t pth;

void* thread1(void*) {
    //app->exec();
    while(true) {
        usleep(100000);
        QApplication::processEvents();
    }
}

char tmp[10] = "";
char* tmp1[] = {tmp};
int argc1=1;

xavna_mock_ui::xavna_mock_ui() {
    bool createApp = (QCoreApplication::instance()==NULL);
    if(createApp) {
        fprintf(stderr,"NO QAPPLICATION FOUND; STARTING QAPPLICATION IN BACKGROUND THREAD\n");
        fflush(stderr);
        app = new QApplication(argc1,tmp1);
    }

    xavna_mock_ui_dialog* wnd=new xavna_mock_ui_dialog();
    this->wnd = wnd;
    wnd->show();
    if(createApp) {
        QMetaObject::invokeMethod(qApp, "quit", Qt::QueuedConnection);
        app->exec();
        pthread_create(&pth, NULL, &thread1, NULL);
    }
}

xavna_mock_ui::~xavna_mock_ui() {
    xavna_mock_ui_dialog* wnd = (xavna_mock_ui_dialog*)this->wnd;
    wnd->close();
}

void xavna_mock_ui::set_cb(const xavna_ui_changed_cb& cb) {
    xavna_mock_ui_dialog* wnd = (xavna_mock_ui_dialog*)this->wnd;
    wnd->cb = cb;
}

