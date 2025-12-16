#pragma once
#include <rtc/rtc.hpp>
#include <string>
#include <memory>
#include <windows.h>
#include <winhttp.h>

class CNetwork
{
public:
    CNetwork(HWND hWnd, std::string code);
    ~CNetwork();

    void setupIce();

    void startSend();
    void startReceive();

    void sendLoop();
    void receiveLoop();

    std::string http_request(
        const std::wstring& host,
        INTERNET_PORT port,
        const std::wstring& method,
        const std::wstring& path,
        const std::string& body
    );

    void postLog(const std::string& msg);

    HWND m_hWnd = nullptr;
    std::string m_strCode;

    rtc::Configuration m_config;
    std::shared_ptr<rtc::PeerConnection> m_pc;

    std::string path;
};

static UINT __cdecl SendThread(LPVOID pParam);
static UINT __cdecl ReceiveThread(LPVOID pParam);