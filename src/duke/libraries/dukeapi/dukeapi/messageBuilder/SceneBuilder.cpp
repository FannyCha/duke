/*
 * SceneBuilder.cpp
 *
 *  Created on: 27 mars 2012
 *      Author: Guillaume Chatelet
 */

#include "SceneBuilder.h"
#include "MeshBuilder.h"
#include "ShaderBuilder.h"
#include "ParameterBuilder.h"

#include <player.pb.h>

#include <boost/bind.hpp>
#include <boost/algorithm/string.hpp>

#include <sstream>
#include <set>
#include <map>

#include <functional>

using namespace std;
using namespace google::protobuf;
using namespace google::protobuf::serialize;
using namespace duke::playlist;
using namespace duke::protocol;


static void setRange(FrameRange *pRange, uint32_t first, uint32_t last) {
    pRange->set_first(first);
    pRange->set_last(last);
}

static vector<string> getTracks(const Playlist &playlist) {
    set<string> tracks;
    for (int i = 0; i < playlist.shot_size(); ++i)
        tracks.insert(playlist.shot(i).track());
    vector<string> sorted;
    copy(tracks.begin(), tracks.end(), back_inserter(sorted));
    sort(sorted.begin(), sorted.end());
    return sorted;
}

static inline Display getDefaultDisplay() {
    Display display;
    display.set_colorspace(Display::SRGB);
    return display;
}

Display update(const Display &local, const Display &with) {
    Display copy(local);
    if (!copy.has_colorspace() && with.has_colorspace())
        copy.set_colorspace(with.colorspace());
    return copy;
}

void normalize(Playlist &playlist) {
    const Display defaultDisplay = getDefaultDisplay();
    const Display globalDisplay = playlist.has_display() ? update(playlist.display(), defaultDisplay) : defaultDisplay;
    playlist.mutable_display()->CopyFrom(globalDisplay);
    for (int i = 0; i < playlist.shot_size(); ++i) {
        Shot &current = *playlist.mutable_shot(i);
        Display &display = *current.mutable_display();
        display.CopyFrom(current.has_display() ? update(current.display(), globalDisplay) : globalDisplay);
        display.mutable_shader()->Clear();
        switch (display.colorspace()) {
            case Display::LIN:
                display.add_shader("lintosrgb");
                break;
            case Display::LOG:
                display.add_shader("cineontolin");
                display.add_shader("lintosrgb");
                break;
            case Display::SRGB:
                break;
        }
    }
}

static AutomaticParameter automaticTexDim(const string& name) {
    AutomaticParameter param;
    param.set_name(name);
    param.set_type(AutomaticParameter::FLOAT3_TEX_DIM);
    return param;
}

static void setClipSamplingSource(SamplingSource *pSource, const string &clipName) {
    pSource->set_type(SamplingSource::CLIP);
    pSource->set_name(clipName);
}

static string tweakName(const string& name, const string &clipName) {
    if (clipName.empty())
        return name;
    return clipName + "|" + name;
}

static AutomaticParameter automaticClipSource(const string& name, const string &clipName) {
    AutomaticParameter param = automaticTexDim(tweakName(name, clipName));
    setClipSamplingSource(param.mutable_samplingsource(), clipName);
    return param;
}

static void addSamplerState(StaticParameter &param, SamplerState_Type type, SamplerState_Value value) {
    SamplerState &state = *param.add_samplerstate();
    state.set_type(type);
    state.set_value(value);
}

static StaticParameter staticClipSampler(const string& name, const string &clipName) {
    StaticParameter param;
    param.set_name(tweakName(name, clipName));
    param.set_type(StaticParameter::SAMPLER);
    addSamplerState(param, SamplerState::MIN_FILTER, SamplerState::TEXF_POINT);
    addSamplerState(param, SamplerState::MAG_FILTER, SamplerState::TEXF_POINT);
    addSamplerState(param, SamplerState::WRAP_S, SamplerState::WRAP_BORDER);
    addSamplerState(param, SamplerState::WRAP_T, SamplerState::WRAP_BORDER);
    setClipSamplingSource(param.mutable_samplingsource(), clipName);
    return param;
}

static string shaderName(const vector<string> effects) {
    ostringstream out;
    out << "ps_";
    copy(effects.begin(), effects.end(), ostream_iterator<string>(out, "_"));
    return out.str();
}

struct SceneBuilder {
    SceneBuilder(const Playlist &playlist) {
        const vector<string> tracks = getTracks(playlist);
        scene.mutable_track()->Reserve(tracks.size());
        for (vector<string>::const_iterator itr = tracks.begin(), end = tracks.end(); itr != end; ++itr) {
            const string &name(*itr);
            Track *pTrack = scene.add_track();
            pTrack->set_name(name);
            m_Track[name] = pTrack;
        }
    }

    void handleShot(const Shot &shot) {
        Track &track = *m_Track[shot.track()];
        Clip &clip = *track.add_clip();
        ostringstream msg;
        msg << track.name() << '/' << track.clip_size();
        clip.set_name(msg.str());
        setMedia(shot, clip);
        setGrading(shot, clip);
    }

    template<typename T>
    void packAndShare(const T& message) {
        result.push_back(google::protobuf::serialize::packAndShare(message));
//        message.PrintDebugString();
    }

    vector<SharedHolder> finish() {
        packAndShare(scene);
        return result;
    }
private:
    void setMedia(const Shot &shot, Clip &clip) {
        Media &media = *clip.mutable_media();
        if (shot.has_in() && shot.has_out()) {
            media.set_type(Media::IMAGE_SEQUENCE);
            setRange(media.mutable_source(), shot.in(), shot.out());
        } else {
            media.set_type(Media::SINGLE_IMAGE);
        }
        media.set_filename(shot.media());
        if (shot.has_reverse())
            media.set_reverse(shot.reverse());
        setRange(clip.mutable_record(), shot.recin(), shot.recout());
    }

    void setGrading(const Shot &shot, Clip &clip) {
        Grading &grading = *clip.mutable_grade();
        RenderPass &pass = *grading.add_pass();
        pass.set_clean(true);
        pass.add_meshname(MeshBuilder::plane);
        Effect &effect = *pass.mutable_effect();
        effect.set_vertexshadername("vs");
        vector<string> effects;
        const Display &display = shot.display();
        effects.push_back("rgbatobgra");
        if (boost::iends_with(shot.media(), ".dpx"))
            effects.push_back("tenbitunpackfloat");
        copy(display.shader().begin(), display.shader().end(), back_inserter(effects));
        const string psName = generateShader(effects);
        effect.set_pixelshadername(psName);
        packAndShare(automaticClipSource(ParameterBuilder::image_dim, clip.name()));
        packAndShare(staticClipSampler("sampler", clip.name()));
    }

    string generateShader(const vector<string> &effects) {
        const string name = shaderName(effects);
        if (shaders.find(name) == shaders.end()) {
            Shader ps = ShaderBuilder::pixelShader(name, NULL);
            int i = 1;
            for (vector<string>::const_iterator itr = effects.begin(), end = effects.end(); itr != end; ++itr, ++i)
                ShaderBuilder::addShadingNode(ps, *itr, i);
            packAndShare(ps);
            shaders.insert(name);
        }
        return name;
    }

    set<string> shaders;
    map<string, Track*> m_Track;
    Scene scene;
    vector<SharedHolder> result;
};

vector<google::protobuf::serialize::SharedHolder> getMessages(const Playlist &playlist) {
    const RepeatedPtrField<Shot> &shots = playlist.shot();
    SceneBuilder builder(playlist);
    // mesh
    builder.packAndShare(MeshBuilder::buildPlane(MeshBuilder::plane));
    // appending unbound parameters
    builder.packAndShare(automaticTexDim(ParameterBuilder::display_dim));
    builder.packAndShare(ParameterBuilder::staticFloat(ParameterBuilder::display_mode, 2)); // fit to display X
    builder.packAndShare(ParameterBuilder::staticFloat(ParameterBuilder::image_ratio, 0));
    builder.packAndShare(ParameterBuilder::staticFloat(ParameterBuilder::zoom_ratio, 1));
    builder.packAndShare(ParameterBuilder::staticFloat(ParameterBuilder::pan_x, 0));
    builder.packAndShare(ParameterBuilder::staticFloat(ParameterBuilder::pan_y, 0));

    Shader vs  = ShaderBuilder::vertexShader(ShaderBuilder::default_vertex_shader, fittableTransformVs);
    vs.add_parametername(ParameterBuilder::display_dim);
    vs.add_parametername(ParameterBuilder::image_dim);
    vs.add_parametername(ParameterBuilder::display_mode);
    vs.add_parametername(ParameterBuilder::image_ratio);
    vs.add_parametername(ParameterBuilder::zoom_ratio);
    vs.add_parametername(ParameterBuilder::pan_x);
    vs.add_parametername(ParameterBuilder::pan_y);
    builder.packAndShare(vs);

    for_each(shots.begin(), shots.end(), boost::bind(&SceneBuilder::handleShot, boost::ref(builder), _1));

    return builder.finish();
}
