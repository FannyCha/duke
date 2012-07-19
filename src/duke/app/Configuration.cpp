#include "Configuration.h"

#include <dukeengine/CmdLineOptions.h>
#include <dukeengine/Version.h>
#include <dukeengine/host/io/ImageDecoderFactoryImpl.h>

#include <dukeapi/io/PlaybackReader.h>
#include <dukeapi/io/FileRecorder.h>
#include <dukeapi/io/InteractiveMessageIO.h>
#include <dukeapi/io/UIMessageIO.h>
#include <dukeapi/protobuf_builder/CmdLineParser.h>
#include <dukeapi/messageBuilder/SceneBuilder.h>
#include <dukeapi/QueueMessageIO.h>

#include <player.pb.h>
#include <protocol.pb.h>

#include <boost/filesystem.hpp>

#include <string>
#include <iostream>
#include <fstream>
#include <sstream>

// namespace
namespace po = boost::program_options;
namespace fs = boost::filesystem;
using namespace std;

void setDisplayOptions(boost::program_options::options_description& description, const ::duke::protocol::Renderer& Renderer) {
    ostringstream resolution;
    resolution << Renderer.width() << 'x' << Renderer.height();
    description.add_options() //
    (FULLSCREEN_OPT, "Sets the application to run fullscreen") //
    (BLANKING_OPT, po::value<unsigned int>()->default_value(1), "Blanking count before presentation, up to 4, 0 means immediate and results in tearing effect.") //
    (REFRESHRATE_OPT, po::value<unsigned int>()->default_value(Renderer.refreshrate()), "Forces the screen refresh rate (fullscreen mode)") //
    (RESOLUTION_OPT, po::value<string>()->default_value(resolution.str()), "Sets the dimensions of the display") //
     ;
}

const static string HEADER = "[Configuration] ";

Configuration::Configuration(int argc, char** argv) :
                m_CmdLineOnly("command line only options"), //
                m_Config("configuration options"), //
                m_Display("display options"), //
                m_Interactive("interactive mode options"), //
                m_CmdlineOptionsGroup("Command line options"), //
                m_ConfigFileOptions("Configuration file options"), //
                m_HiddenOptions("hidden options"), //
                m_Mode(NO_OP), //
                m_Port(0) {

    using namespace ::duke::protocol;

    // retrieving configuration file
    string configuration_filename;
    if (argc >= 1) {
        fs::path p(argv[0]);
        p.replace_extension(".ini");
        if (exists(p) && is_regular_file(p))
            configuration_filename = p.string();
    }

    // available on the command line
    m_CmdLineOnly.add_options() //
    (HELP_OPT, "Displays this help") //
    (VERSION_OPT, "Displays the version informations");

    // available in the configuration file and command line
    m_Config.add_options() //
    (PLAYBACK_OPT, po::value<string>(), "Play a recorded session back from file") //
    (RECORD_OPT, po::value<string>(), "Record a session to file") //
    (PORT_OPT, po::value<short>(), "Sets the port number to be used") //
    (CACHESIZE_OPT, po::value<string>()->default_value("50%"), "Cache size for look ahead. Valid units are K,M,G or %") //
    (THREADS_OPT, po::value<size_t>()->default_value(0), "Number of load/decode threads. Cache size must be >0.") //
    (NOGUI_OPT, "Hide all UI elements.");

    // adding display settings
    ::duke::protocol::Renderer renderer;
    setDisplayOptions(m_Display, renderer);

    // adding interactive mode options
    m_Interactive.add_options() //
    (BROWSE_OPT, "Browse mode, act as an image browser") //
    (FRAMERATE_OPT, po::value<unsigned int>()->default_value(25), "Sets the playback framerate") //
    (NOFRAMERATE_OPT, "Reads the playlist as fast as possible. All images are displayed . Testing purpose only.") //
    (NOSKIP_OPT, "Try to keep the framerate but still ensures all images are displayed. Testing purpose only.");

    //adding hidden options
    m_HiddenOptions.add_options()(INPUTS, po::value<vector<string> >(), "input directories, files, sequences, playlists.");

    po::positional_options_description pod;
    pod.add(INPUTS, -1);

    // parsing the command line
    m_CmdlineOptionsGroup.add(m_CmdLineOnly).add(m_Config).add(m_Display).add(m_Interactive);
    boost::program_options::options_description parsingOptions;
    parsingOptions.add(m_CmdlineOptionsGroup).add(m_HiddenOptions);
    po::store(po::command_line_parser(argc, argv).options(parsingOptions).positional(pod).run(), m_Vm);

    // now parsing the configuration file - already stored variables will remain unchanged
    m_ConfigFileOptions.add(m_Config).add(m_Display).add(m_Interactive);
    ifstream configFile(configuration_filename.c_str(), ifstream::in);
    if (configFile.is_open()) {
        cout << HEADER + "Reading options from " << configuration_filename << endl;
        po::store(po::parse_config_file(configFile, m_ConfigFileOptions), m_Vm);
    }
    configFile.close();

    // notifying from incoming new options
    po::notify(m_Vm);

    try {

        if (m_Vm.count(HELP)) {
            displayHelp();
            return;
        }
        if (m_Vm.count(VERSION)) {
            displayVersion();
            return;
        }

        // parse caching options
        const uint64_t cacheSize = parseCache(m_Vm[CACHESIZE].as<string>());
        const size_t threads = m_Vm[THREADS].as<size_t>();
        m_Cache.set_size(cacheSize);
        m_Cache.set_threading(threads);
        m_Cache.clear_region();

        // loading plugins
        m_pImageFactory.reset(new ImageDecoderFactoryImpl());
        const char** listOfExtensions = m_pImageFactory->getAvailableExtensions();

        /**
         * Server mode
         */
        // if port is specified turning into a server
        if (m_Vm.count(PORT)) {
            m_Port = m_Vm[PORT].as<short>();
            m_Mode = SERVER;
            return;
        }

//        /**
//         * Playback mode
//         */
//        if (m_Vm.count(PLAYBACK)) {
//            m_Mode = PLAYBACK;
//            const string filename = m_Vm[PLAYBACK].as<string>();
//            cout << HEADER + "Reading protocol buffer script: " << filename << endl;
//            m_pIO.reset(new PlaybackReader(filename.c_str()));
//            return;
//        }

        /**
         * Interactive mode
         */
        renderer.set_presentinterval(m_Vm[BLANKING].as<unsigned>());
        renderer.set_fullscreen(m_Vm.count(FULLSCREEN) > 0);
        renderer.set_refreshrate(m_Vm[REFRESHRATE].as<unsigned>());
        if (m_Vm.count(RESOLUTION) != 0) {
            string res = m_Vm[RESOLUTION].as<string>();
            replace(res.begin(), res.end(), 'x', ' ');
            replace(res.begin(), res.end(), 'X', ' ');
            istringstream stream(res);
            int width = -1;
            int height = -1;
            stream >> width;
            stream >> height;
            if (stream.bad() || width == -1 || height == -1)
                throw cmdline_exception(string("bad resolution \"") + res + '\"');
            renderer.set_width(width);
            renderer.set_height(height);
        }

        // checking renderer
        if (renderer.presentinterval() > 4)
            throw cmdline_exception(string(BLANKING) + " must be between 0 an 4");

        // checking command line
        const bool browseMode = m_Vm.count(BROWSE);
        const bool hasInputs = m_Vm.count(INPUTS);
        const vector<string> inputs = hasInputs ? m_Vm[INPUTS].as<vector<string> >() : vector<string>();

        if (browseMode) {
            if (inputs.empty() || inputs.size() > 1)
                throw cmdline_exception("You are in browse mode, you must specify one and only one input.");
        } else if (inputs.empty())
            throw cmdline_exception("You should specify at least one input : filename, directory or playlist files.");


        MessageQueue *queue;
        if(m_Vm.count(NOGUI) > 0){
            m_Mode = DUKE;
            m_pIO.reset(new InteractiveMessageIO());
            queue = &dynamic_cast<InteractiveMessageIO*>(m_pIO.get())->m_ToApplicationQueue; // ugly!
        } else {
            m_Mode = DUKEX;
            m_pIO.reset(new UIMessageIO());
            queue = &dynamic_cast<UIMessageIO*>(m_pIO.get())->m_ToApplicationQueue; // ugly!
        }

        IOQueueInserter queueInserter(*queue);
        queueInserter << renderer; // setting renderer

        Engine stop;
        stop.set_action(Engine::RENDER_STOP);
        queueInserter << stop; // stopping rendering for now

        const extension_set validExtensions = extension_set::create(listOfExtensions);
        duke::playlist::Playlist playlist = browseMode ? browseViewerComplete(validExtensions, inputs[0]) : browsePlayer(validExtensions, inputs);

        if (playlist.shot_size() == 0)
            throw runtime_error("No media found, nothing to render. Aborting.");

        if (m_Vm.count(FRAMERATE))
            playlist.set_framerate(m_Vm[FRAMERATE].as<unsigned int>());

        normalize(playlist);

        vector<google::protobuf::serialize::SharedHolder> messages = getMessages(playlist);
        queue->drainFrom(messages);

        {
            duke::protocol::PlaybackState playback;
            const PlaybackState::PlaybackMode mode = m_Vm.count(NOFRAMERATE) > 0 ? PlaybackState::RENDER : //
                                                     m_Vm.count(NOSKIP) > 0 ? PlaybackState::NO_SKIP : PlaybackState::DROP_FRAME_TO_KEEP_REALTIME;
            playback.set_playbackmode(mode);
            playback.set_frameratenumerator(playlist.framerate());
            playback.set_loop(playlist.loop());
            queueInserter << playback;
        }
        if (playlist.has_startframe()) {
            duke::protocol::Transport cue;
            cue.set_type(Transport::CUE);
            cue.mutable_cue()->set_value(playlist.startframe());
            queueInserter << cue;
        }

        Engine start;
        start.set_action(Engine::RENDER_START);
        queueInserter << start;

    } catch (cmdline_exception &e) {
        cout << "invalid command line : " << e.what() << endl << endl;
        displayHelp();
        m_Mode = NO_OP;
    }
}
//
//void Configuration::decorateAndRun(IMessageIO& io, ImageDecoderFactoryImpl &imageDecoderFactory) {
//    if (m_Vm.count(RECORD) > 0) {
//        const string recordFilename = m_Vm[RECORD].as<string>();
//        FileRecorder recorder(recordFilename.c_str(), io);
//        cout << HEADER + "recording session to " << recordFilename << endl;
//        run(recorder, imageDecoderFactory);
//    } else {
//        run(io, imageDecoderFactory);
//    }
//}

void Configuration::displayVersion() {
    cout << getVersion("Duke") << endl;
}

void Configuration::displayHelp() {
    cout << "Usage: " << endl;
    cout << "\tOpen a file in browse mode            duke -b /path/to/img.1234.jpg" << endl;
    cout << "\tOpen a folder in browse mode          duke -b /path/to" << endl;
    cout << "\tOpen a sequence containing file       duke -s /path/to/img.1234.jpg" << endl;
    cout << "\tOpen a single file                    duke foo.jpg" << endl;
    cout << "\tOpen a playlist                       duke playlist.ppl" << endl;
    cout << "\tParse current folder for sequences    duke ." << endl;
    cout << "\tParse several folders for sequences   duke /path/to/dir1 /path/to/dir2" << endl << endl;
    cout << m_CmdlineOptionsGroup << endl;
}

IMessageIO& Configuration::io() const {
    if (m_pIO.get() == NULL)
        throw exception();
    return *m_pIO;
}

ImageDecoderFactory& Configuration::imageFactory() const {
    if (m_pImageFactory.get() == NULL)
        throw exception();
    return *m_pImageFactory;
}
