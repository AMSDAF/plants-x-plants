/* Envia texto Unicode à janela de teste, sem controlar outras janelas do usuário. */
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
void check_queue_codepoint(void *window, unsigned int codepoint)
{
    HWND handle=(HWND)window;
    if (codepoint<=0xffff) PostMessageW(handle,WM_CHAR,codepoint,1);
    else {
        codepoint-=0x10000;
        PostMessageW(handle,WM_CHAR,0xd800+(codepoint>>10),1);
        PostMessageW(handle,WM_CHAR,0xdc00+(codepoint&0x3ff),1);
    }
}
#endif
