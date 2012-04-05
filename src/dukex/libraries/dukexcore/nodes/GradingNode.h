#ifndef GRADINGNODE_H
#define GRADINGNODE_H

#include <dukexcore/dkxINode.h>
#include <dukexcore/dkxSessionDescriptor.h>
#include <dukeapi/messageBuilder/ParameterBuilder.h>
#include <dukeapi/protobuf_builder/SceneBuilder.h>
#include <player.pb.h>


class GradingNode : public INode {

public:
    typedef boost::shared_ptr<GradingNode> ptr;
    GradingNode() :
        INode("fr.mikrosimage.dukex.grading") {
    }
    virtual ~GradingNode() {
    }

public:
    bool setZoom(double zoomRatio) {
        try {
            MessageQueue queue;
            addStaticFloatParam(queue, "zoom", zoomRatio);
            session()->sendMsg(queue);
        } catch (std::exception & e) {
            std::cerr << e.what() << std::endl;
            return false;
        }
        return true;
    }

    bool setPan(double panX, double panY) {
        try {
            MessageQueue queue;
            addStaticFloatParam(queue, "panX", panX);
            addStaticFloatParam(queue, "panY", panY);
            session()->sendMsg(queue);
        } catch (std::exception & e) {
            std::cerr << e.what() << std::endl;
            return false;
        }
        return true;
    }

    bool setColorspace(::duke::playlist::Display::ColorSpace colorspace) {
        try {
            MessageQueue queue;
            IOQueueInserter inserter(queue);

            // Push engine stop
            ::duke::protocol::Engine stop;
            stop.set_action(::duke::protocol::Engine::RENDER_STOP);
            inserter << stop;

            // store current frame
            duke::protocol::Transport cuestore;
            cuestore.set_type(duke::protocol::Transport::STORE);
            push(queue, cuestore);

            // Get playlist from session
            ::duke::playlist::Playlist & playlist = session()->descriptor().playlist();

            // Set the right colorspace
            for(int i=0;i<playlist.shot_size();++i)
                playlist.mutable_shot(i)->mutable_display()->set_colorspace(colorspace);
            normalize(playlist);
            session()->descriptor().setPlaylist(playlist);
            std::vector<google::protobuf::serialize::SharedHolder> messages = getMessages(playlist);
            queue.drainFrom(messages);

            // go to previously stored frame
            duke::protocol::Transport cue;
            cue.set_type(duke::protocol::Transport::CUE_STORED);
            push(queue, cue);

            // Push engine start
            ::duke::protocol::Engine start;
            start.set_action(::duke::protocol::Engine::RENDER_START);
            inserter << start;

            // Send
            session()->sendMsg(queue);
        } catch (std::exception & e) {
            std::cerr << e.what() << std::endl;
            return false;
        }
        return true;
    }

    bool setBackground(const std::string & rgba) {
        try {
//            MessageQueue queue;
//            IOQueueInserter inserter(queue);
//            session()->sendMsg(queue);
        } catch (std::exception & e) {
            std::cerr << e.what() << std::endl;
            return false;
        }
        return true;
    }

};

#endif // GRADINGNODE_H
