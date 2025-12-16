#include "pch.h"
#include <rtc/rtc.hpp>
#include <windows.h>
#include <winhttp.h>
#include <string>
#include <memory>
#include <thread>
#include <sstream>
#include <fstream>

#include "CNetwork.h"
#include "json.hpp"

#pragma comment(lib, "winhttp.lib")

using namespace std;
using json = nlohmann::json;

#define WM_ADD_LOG (WM_USER + 1)

#define CHUNK_SIZE (32 * 1024)
#define HIGH_WATER   (64 * 1024 * 1024)

    CNetwork::CNetwork(HWND hWnd, string code)
        : m_hWnd(hWnd), m_strCode(code)
    {
    }

    CNetwork::~CNetwork()
    {
        m_pc.reset();
    }

    void CNetwork::setupIce()
    {
        rtc::Configuration cfg;

        rtc::IceServer turn("turn:54.206.48.157:3478");
        turn.username = "ID";
        turn.password = "PASSWORD";
        cfg.iceServers.push_back(turn);

        m_config = cfg;
    }

    void CNetwork::startSend()
    {
        postLog("Send Thread Starting...");

        CWinThread* th = AfxBeginThread(SendThread, this);
        if (!th) {
            postLog("Start Failed!");
        }
    }

    void CNetwork::startReceive()
    {
        AfxBeginThread(ReceiveThread, this);
    }

    static UINT __cdecl SendThread(LPVOID pParam)
    {
        static_cast<CNetwork*>(pParam)->sendLoop();
        return 0;
    }

    static UINT __cdecl ReceiveThread(LPVOID pParam)
    {
        static_cast<CNetwork*>(pParam)->receiveLoop();
        return 0;
    }

    void CNetwork::sendLoop()
    {
        postLog("[send] start");

        m_pc = std::make_shared<rtc::PeerConnection>(m_config);

        auto dc = m_pc->createDataChannel("file");

        dc->onOpen([this,dc]() {
            postLog("[send] DataChannel open");

            std::thread([this, dc]() {
                std::ifstream file(path, std::ios::binary);
                if (!file) {
                    postLog("[send] file open failed");
                    return;
                }

                file.seekg(0, ios::end);
                uint64_t length = file.tellg();
                file.seekg(0, ios::beg);

                string fileName = path.substr(path.find_last_of('/\\') + 1);

                string meta = "META\n" + fileName + "\n" + to_string(length);
                dc->send(meta);

                std::vector<std::byte> buffer(CHUNK_SIZE);

                size_t totalRead = 0;
                auto lastLog = std::chrono::steady_clock::now();
                while (file) {                    
                    if (dc->bufferedAmount() > HIGH_WATER) {
                        std::this_thread::sleep_for(1s);
                        continue;
                    }
                    file.read(reinterpret_cast<char*>(buffer.data()), CHUNK_SIZE);
                    size_t readBytes = file.gcount();
                    if (readBytes <= 0) break;

                    rtc::binary data(
                        buffer.data(),
                        buffer.data() + readBytes
                    );

                    dc->send(data);

                    totalRead += readBytes;
                    auto now = std::chrono::steady_clock::now();
                    if (now - lastLog >= std::chrono::milliseconds(300)) {
                        postLog(
                            "[send] Progress : " + std::to_string(totalRead) +
                            " / " + std::to_string(length) +
                            "   " + to_string(totalRead*100/length) + "%"
                        );
                        lastLog = now;
                    }
                }

                dc->send(string("EOF"));

                postLog("[send] file send complete");
            }).detach();

        });

        dc->onMessage([this](auto msg) {
            if (holds_alternative<string>(msg))
                postLog("[send] recv: " + get<string>(msg));
        });

        m_pc->onGatheringStateChange([this](auto state) {
            if (state == rtc::PeerConnection::GatheringState::Complete) {
                auto desc = m_pc->localDescription();
                if (desc) {
                    json j;
                    j["key"] = m_strCode;
                    j["offer"] = string(*desc);
                    j["candidates"] = json::array();
                    string offer = http_request(
                        L"54.206.48.157", 5000,
                        L"POST", L"/session",
                        j.dump()
                    );

                    while (1) {
                        postLog("[send] waiting for response...");

                        string answer = http_request(
                            L"54.206.48.157", 5000,
                            L"GET", L"/session/" +
                            std::wstring(m_strCode.begin(), m_strCode.end()) +
                            L"/answer",
                            ""
                        );
                        if (answer.length() < 30) {
                            Sleep(3);  continue;
                        }
                        
                        json j_ans = json::parse(answer);
                        m_pc->setRemoteDescription(rtc::Description(j_ans["answer"]));
                        break;
                    }
                }
            }
        });

        m_pc->setLocalDescription();

        while (true)
            Sleep(1000);
    }

    enum class RecvState {
        WaitMeta,
        Receiving,
        Done
    };


    void CNetwork::receiveLoop()
    {
        postLog("[recv] start");
        m_pc = std::make_shared<rtc::PeerConnection>(m_config);

        m_pc->onDataChannel([this](auto dc) {

            auto state = std::make_shared<RecvState>(RecvState::WaitMeta);
            auto file = std::make_shared<std::ofstream>();
            auto remainSize = std::make_shared<uint64_t>(0);
            auto fileName = std::make_shared<std::string>();

            dc->onOpen([this, dc]() {
                postLog("[recv] DataChannel open");
                dc->send("READY");
            });

            uint64_t totalRead = 0;
            auto lastLog = std::chrono::steady_clock::now();
            dc->onMessage([=](auto msg) mutable {
                if (*state == RecvState::WaitMeta) {
                    if (!std::holds_alternative<std::string>(msg))
                        return;

                    const std::string& text = std::get<std::string>(msg);
                    if (text.rfind("META\n", 0) != 0)
                        return;

                    std::istringstream iss(text);
                    std::string line;
                    getline(iss, line);
                    getline(iss, *fileName);

                    std::string sizeStr;
                    getline(iss, sizeStr);
                    *remainSize = std::stoull(sizeStr);

                    PWSTR downloadPath = nullptr;
                    SHGetKnownFolderPath(
                        FOLDERID_Downloads,
                        0,
                        NULL,
                        &downloadPath
                    );

                    std::string fullPath =
                        std::string(CT2CA(downloadPath)) + "\\" + *fileName;

                    CoTaskMemFree(downloadPath);

                    file->open(fullPath, std::ios::binary);
                    if (!file->is_open()) {
                        postLog("[recv] file open failed");
                        return;
                    }

                    postLog("[recv] Receiving: " + fullPath +
                        " (" + std::to_string(*remainSize) + " bytes)");

                    *state = RecvState::Receiving;
                    return;
                }

                if (*state == RecvState::Receiving) {
                    if (std::holds_alternative<std::string>(msg)) {
                        const std::string& text = std::get<std::string>(msg);

                        if (text == "EOF") {
                            file->close();
                            *state = RecvState::Done;
                            postLog("[recv] File receive complete");
                            dc->send("RECV_OK");
                            return;
                        }
                    }
                    if (std::holds_alternative<rtc::binary>(msg)) {
                        const auto& bin = std::get<rtc::binary>(msg);

                        file->write(
                            reinterpret_cast<const char*>(bin.data()),
                            bin.size()
                        );

                        totalRead += bin.size();

                        auto now = std::chrono::steady_clock::now();
                        if (now - lastLog >= std::chrono::milliseconds(300)) {
                            postLog(
                                "[recv] Progress : " + std::to_string(totalRead) +
                                " / " + std::to_string(*remainSize) +
                                "   " + to_string(totalRead * 100 / *remainSize) + "%"
                            );
                            lastLog = now;
                        }
                    }
                }

                if (*state == RecvState::Done) {
                    dc->close();
                }
            });
        });

        string offer = http_request(
            L"54.206.48.157", 5000,
            L"GET", 
            L"/session/" +
            std::wstring(m_strCode.begin(), m_strCode.end()),
            ""
        );

        json j = json::parse(offer);

        postLog("[recv] offer received");

        m_pc->setRemoteDescription(rtc::Description(j["offer"]));
        m_pc->setLocalDescription();

        m_pc->onGatheringStateChange([this](auto state) {
            if (state == rtc::PeerConnection::GatheringState::Complete) {
                auto desc = m_pc->localDescription();
                if (!desc) return;

                std::string sdp = std::string(*desc);
                std::istringstream iss(sdp);
                std::ostringstream oss;
                std::string line;
                while (std::getline(iss, line)) {
                    if (line.rfind("a=setup:actpass", 0) == 0) {
                        oss << "a=setup:active\n";
                    }
                    else {
                        oss << line << "\n";
                    }
                }
                sdp = oss.str();

                json j;
                j["key"] = m_strCode;
                j["answer"] = sdp;
                j["candidates"] = json::array();

                http_request(
                    L"54.206.48.157", 5000,
                    L"POST", L"/session/" +
                    std::wstring(m_strCode.begin(), m_strCode.end()) +
                    L"/answer",
                    j.dump()
                );

                postLog("[recv] answer sent");
            }
        });

        while (true)
            Sleep(1);
    }

    string CNetwork::http_request(
        const wstring& host,
        INTERNET_PORT port,
        const wstring& method,
        const wstring& path,
        const string& body
    ) {
        string response;

        HINTERNET hSession = WinHttpOpen(
            L"MyApp/1.0",
            WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
            WINHTTP_NO_PROXY_NAME,
            WINHTTP_NO_PROXY_BYPASS,
            0
        );
        if (!hSession) return "";

        HINTERNET hConnect = WinHttpConnect(
            hSession,
            host.c_str(),
            port,
            0
        );
        if (!hConnect) goto cleanup1;

        HINTERNET hRequest = WinHttpOpenRequest(
            hConnect,
            method.c_str(),
            path.c_str(),
            NULL,
            WINHTTP_NO_REFERER,
            WINHTTP_DEFAULT_ACCEPT_TYPES,
            0
        );
        if (!hRequest) goto cleanup2;

        WinHttpAddRequestHeaders(
            hRequest,
            L"Content-Type: application/json\r\n",
            -1,
            WINHTTP_ADDREQ_FLAG_ADD
        );

        BOOL ok = WinHttpSendRequest(
            hRequest,
            WINHTTP_NO_ADDITIONAL_HEADERS,
            0,
            body.empty() ? WINHTTP_NO_REQUEST_DATA : (LPVOID)body.data(),
            body.size(),
            body.size(),
            0
        );
        if (!ok) goto cleanup3;

        ok = WinHttpReceiveResponse(hRequest, NULL);
        if (!ok) goto cleanup3;

        DWORD size = 0;
        do {
            WinHttpQueryDataAvailable(hRequest, &size);
            if (!size) break;

            string buffer(size, 0);
            DWORD read = 0;
            WinHttpReadData(hRequest, buffer.data(), size, &read);
            buffer.resize(read);
            response += buffer;
        } while (size > 0);

    cleanup3:
        WinHttpCloseHandle(hRequest);
    cleanup2:
        WinHttpCloseHandle(hConnect);
    cleanup1:
        WinHttpCloseHandle(hSession);

        return response;
    }

    void CNetwork::postLog(const string& msg)
    {
        if (!m_hWnd) return;
        auto* text = new string(msg + "\n");
        PostMessage(m_hWnd, WM_ADD_LOG, 0, (LPARAM)text);
    }
