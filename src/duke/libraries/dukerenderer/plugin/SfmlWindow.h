#ifndef SFMLWINDOW_H_
#define SFMLWINDOW_H_

#include "IRenderer.h"
#include <player.pb.h>
#include <SFML/Window.hpp>
#include <boost/thread.hpp>
#include <boost/function.hpp>
#include <memory>

#define SFML_WINDOW_TITLE "Duke Player"

// forward declaration
class RendererSuite;

typedef boost::function<IRenderer* (const duke::protocol::Renderer&, sf::Window&, const RendererSuite&)> RendererFactoryFunc;

class SfmlWindow {
private:
    bool m_bWindowCreated;
    sf::Window m_Window;
    std::auto_ptr<IRenderer> m_pRenderer;
    boost::mutex m_Mutex;
    boost::condition_variable m_Condition;
    boost::thread m_MainLoopThread;

public:
    SfmlWindow(const RendererFactoryFunc&, duke::protocol::Renderer, const RendererSuite&);
    ~SfmlWindow();

    void waitForMainLoopEnd();
};

#endif /* SFMLWINDOW_H_ */