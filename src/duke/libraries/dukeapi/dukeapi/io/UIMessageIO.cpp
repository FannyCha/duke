#include "UIMessageIO.h"
#include <dukeapi/messageBuilder/QuitBuilder.h>
#include <protocol.pb.h>
#include <player.pb.h>
#include <iostream>
#include <iomanip>
#include <cassert>

using namespace ::duke::protocol;
using namespace ::google::protobuf::serialize;
using namespace std;

namespace {

Transport MAKE(const Transport_TransportType type //
,
               const int value = -1 //
               ,
               const bool cueRelative = false //
               ,
               const bool cueClip = false) {
    Transport transport;
    transport.set_type(type);
    if (type == Transport::CUE) {
        if (!cueRelative && value < 0)
            throw std::runtime_error("can't cue to a negative frame");
        Transport_Cue *cue = transport.mutable_cue();
        cue->set_cueclip(cueClip);
        cue->set_cuerelative(cueRelative);
        cue->set_value(value);
    }
    return transport;
}

#define PUSH(X) ::push(m_ToApplicationQueue,X)

#define PUSH_RETRIEVE(X) ::push(m_ToApplicationQueue,X,google::protobuf::serialize::MessageHolder::RETRIEVE)

} // namespace

UIMessageIO::UIMessageIO() :
                m_bPlay(false), m_iFitMode(0) {
}

UIMessageIO::~UIMessageIO() {
}

bool UIMessageIO::tryPop(SharedHolder& holder) {
    return m_ToApplicationQueue.tryPop(holder);
}

void UIMessageIO::waitPop(SharedHolder& holder) {
    m_ToApplicationQueue.waitPop(holder);
}

void UIMessageIO::push(const SharedHolder& pHolder) {
    using namespace ::duke::protocol;

    if (!pHolder)
        throw std::runtime_error("Trying to interact with a NULL message");
    const MessageHolder &holder = *pHolder;

    // we are taking into account only Event type messages
    if (!isType<Event>(holder)){
        PUSH(*unpack(holder));
        return;
    }

    Event event;
    unpack(holder, event);
    switch (event.type()) {
        case Event::KEYPRESSED: {
            switch (event.keyevent().code()) {
                case KeyEvent::Space:
                    m_bPlay = !m_bPlay;
                    if (m_bPlay)
                        PUSH(MAKE(Transport::PLAY));
                    else
                        PUSH(MAKE(Transport::STOP));
                    break;
                case KeyEvent::Left:
                    m_bPlay = false;
                    if (event.keyevent().shift())
                        PUSH(MAKE(Transport::CUE, -100, true));
                    else
                        PUSH(MAKE(Transport::CUE, -1, true));
                    break;
                case KeyEvent::Right:
                    m_bPlay = false;
                    if (event.keyevent().shift())
                        PUSH(MAKE(Transport::CUE, 100, true));
                    else
                        PUSH(MAKE(Transport::CUE, 1, true));
                    break;
                case KeyEvent::Home:
                    m_bPlay = false;
                    PUSH(MAKE(Transport::CUE_FIRST));
                    break;
                case KeyEvent::End:
                    m_bPlay = false;
                    PUSH(MAKE(Transport::CUE_LAST));
                    break;
                case KeyEvent::Escape:
                    m_ToApplicationQueue.push(make_shared(QuitBuilder::success()));
                    break;
                case KeyEvent::PageUp:
                    PUSH(MAKE(Transport::CUE, 1, true, true));
                    break;
                case KeyEvent::PageDown:
                    PUSH(MAKE(Transport::CUE, -1, true, true));
                    break;
                case KeyEvent::F: {
                    m_iFitMode = (m_iFitMode + 1) % 3; // 1:1 / fit display H / fit display W
                    StaticParameter displayMode;
                    displayMode.set_name("displayMode");
                    displayMode.set_type(StaticParameter_Type_FLOAT);
                    displayMode.add_floatvalue(m_iFitMode);
                    PUSH(displayMode);
                    break;
                }
                case KeyEvent::G: {
                    PUSH(MAKE(Transport::CUE, atoi(m_ssSeek.str().c_str()), false, false));
                    m_ssSeek.str("");
                    m_ssSeek.clear();
                    break;
                }
                case KeyEvent::H: {
                    cout << "SHORTCUTS" << endl;
                    cout << setw(15) << "[H]" << setw(20) << "Display this help" << endl;
                    cout << setw(15) << "[I]" << setw(20) << "Display image info" << endl;
                    cout << setw(15) << "[F]" << setw(20) << "Toggle fit mode" << endl;
                    cout << setw(15) << "[->]" << setw(20) << "Next frame" << endl;
                    cout << setw(15) << "[<-]" << setw(20) << "Previous frame" << endl;
                    cout << setw(15) << "[shift]+[->]" << setw(20) << "Go to frame + 100" << endl;
                    cout << setw(15) << "[shift]+[<-]" << setw(20) << "Go to frame - 100" << endl;
                    cout << setw(15) << "[PgUp]" << setw(20) << "Next Shot" << endl;
                    cout << setw(15) << "[PgDown]" << setw(20) << "Previous Shot" << endl;
                    cout << setw(15) << "[Home]" << setw(20) << "First frame" << endl;
                    cout << setw(15) << "[End]" << setw(20) << "Last frame" << endl;
                    cout << setw(15) << "[Esc]" << setw(20) << "Quit" << endl;
                    break;
                }
                case KeyEvent::D: {
                    {
                        Info info;
                        info.set_content(Info::PLAYBACKSTATE);
                        PUSH(info);
                    }
                    {
                        Info info;
                        info.set_content(Info::CACHESTATE);
                        PUSH(info);
                    }
                    {
                        Info info;
                        info.set_content(Info::EXTENSIONS);
                        PUSH(info);
                    }
                    {
                        Info info;
                        info.set_content(Info::IMAGEINFO);
                        PUSH(info);
                    }
                    break;
                }
                case KeyEvent::I: {
                    Debug d;
                    d.add_line("IMAGE INFO");
                    d.add_line("    current frame: %0");
                    d.add_line("    associated file(s): %1");
                    d.add_line("    FPS: %2");
                    d.add_content(Debug::FRAME);
                    d.add_content(Debug::FILENAMES);
                    d.add_content(Debug::FPS);
                    PUSH(d);
                    break;
                }
                case KeyEvent::Num0:
                case KeyEvent::Numpad0:
                    m_ssSeek << 0;
                    break;
                case KeyEvent::Num1:
                case KeyEvent::Numpad1:
                    m_ssSeek << 1;
                    break;
                case KeyEvent::Num2:
                case KeyEvent::Numpad2:
                    m_ssSeek << 2;
                    break;
                case KeyEvent::Num3:
                case KeyEvent::Numpad3:
                    m_ssSeek << 3;
                    break;
                case KeyEvent::Num4:
                case KeyEvent::Numpad4:
                    m_ssSeek << 4;
                    break;
                case KeyEvent::Num5:
                case KeyEvent::Numpad5:
                    m_ssSeek << 5;
                    break;
                case KeyEvent::Num6:
                case KeyEvent::Numpad6:
                    m_ssSeek << 6;
                    break;
                case KeyEvent::Num7:
                case KeyEvent::Numpad7:
                    m_ssSeek << 7;
                    break;
                case KeyEvent::Num8:
                case KeyEvent::Numpad8:
                    m_ssSeek << 8;
                    break;
                case KeyEvent::Num9:
                case KeyEvent::Numpad9:
                    m_ssSeek << 9;
                    break;
                default:
                    break;
            }
            break;
        }
        case Event::CLOSED:
            m_ToApplicationQueue.push(make_shared(QuitBuilder::success()));
            break;
        default:
            break;
    }
}
