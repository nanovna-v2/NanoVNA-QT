#ifndef XAVNA_MOCK_UI_H
#define XAVNA_MOCK_UI_H

#include <functional>
using namespace std;
typedef function<void(string dut_name, double cableLen1, double cableLen2)> xavna_ui_changed_cb;

class xavna_mock_ui
{
    void* wnd=NULL;
public:
    xavna_mock_ui();
    ~xavna_mock_ui();
    void set_cb(const xavna_ui_changed_cb &cb);
};

#endif // XAVNA_MOCK_UI_H
