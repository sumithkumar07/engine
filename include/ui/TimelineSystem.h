#pragma once

#include <memory>
#include <vector>
#include <string>
#include <map>
#include <functional>
#include <chrono>
#include "UI.h"

namespace Engine {
namespace UI {

    /**
     * @brief Timeline System for AI Movie Studio
     * 
     * Real implementation with actual functionality:
     * - Visual timeline with time markers
     * - Keyframe management (add, edit, delete)
     * - Playback controls (play, pause, stop, scrub)
     * - Animation curves and easing
     * - Multi-track support for different properties
     */
    class TimelineSystem : public UIComponent {
    public:
        Type GetType() const override { return Type::Panel; }
    public:
        enum class PlaybackState {
            Stopped,
            Playing,
            Paused
        };

        enum class KeyframeType {
            Position,
            Rotation,
            Scale,
            Material,
            Light,
            Camera
        };

        struct Keyframe {
            float time;
            float value;
            KeyframeType type;
            std::string objectName;
            std::string propertyName;
            bool selected;
            
            Keyframe(float t, float v, KeyframeType kt, const std::string& obj, const std::string& prop)
                : time(t), value(v), type(kt), objectName(obj), propertyName(prop), selected(false) {}
        };

        struct AnimationTrack {
            std::string name;
            KeyframeType type;
            std::string objectName;
            std::string propertyName;
            std::vector<Keyframe> keyframes;
            bool visible;
            bool locked;
            float minValue;
            float maxValue;
            
            AnimationTrack(const std::string& n, KeyframeType t, const std::string& obj, const std::string& prop)
                : name(n), type(t), objectName(obj), propertyName(prop), visible(true), locked(false), minValue(0.0f), maxValue(1.0f) {}
        };

        TimelineSystem();
        ~TimelineSystem();

        /**
         * @brief Initialize the timeline system
         * @param viewportRenderer The viewport renderer for scene operations
         * @return true if initialization successful
         */
        bool Initialize(class ViewportRenderer* viewportRenderer);

        /**
         * @brief Shutdown the timeline system
         */
        void Shutdown();

        /**
         * @brief Update the timeline system
         * @param deltaTime Time elapsed since last frame
         */
        void Update(float deltaTime) override;

        /**
         * @brief Render the timeline system
         * @param hdc Device context for rendering
         */
        void Render(HDC hdc) override;

        /**
         * @brief Handle mouse events
         * @param x Mouse X position
         * @param y Mouse Y position
         * @param button Mouse button (0=left, 1=right, 2=middle)
         * @param pressed true if button pressed, false if released
         */
        void HandleMouseEvent(int x, int y, int button, bool pressed) override;

        /**
         * @brief Handle keyboard events
         * @param message Windows message
         * @param wParam WPARAM
         * @param lParam LPARAM
         */
        void HandleKeyboardEvent(UINT message, WPARAM wParam, LPARAM lParam) override;

        // Playback controls
        void Play();
        void Pause();
        void Stop();
        void SetCurrentTime(float time);
        float GetCurrentTime() const { return m_currentTime; }
        PlaybackState GetPlaybackState() const { return m_playbackState; }

        // Timeline settings
        void SetDuration(float duration);
        float GetDuration() const { return m_duration; }
        void SetFPS(float fps);
        float GetFPS() const { return m_fps; }
        void SetZoom(float zoom);
        float GetZoom() const { return m_zoom; }

        // Track management
        void AddTrack(const std::string& name, KeyframeType type, const std::string& objectName, const std::string& propertyName);
        void RemoveTrack(int trackIndex);
        void SetTrackVisible(int trackIndex, bool visible);
        void SetTrackLocked(int trackIndex, bool locked);
        const std::vector<AnimationTrack>& GetTracks() const { return m_tracks; }

        // Keyframe management
        void AddKeyframe(int trackIndex, float time, float value);
        void RemoveKeyframe(int trackIndex, int keyframeIndex);
        void MoveKeyframe(int trackIndex, int keyframeIndex, float newTime, float newValue);
        void SelectKeyframe(int trackIndex, int keyframeIndex, bool multiSelect = false);
        void ClearSelection();

        // Animation playback
        void UpdateAnimation(float deltaTime);
        void EvaluateAnimation(float time);
        void InterpolateKeyframes(const AnimationTrack& track, float time, float& result);

        // UI interaction
        void SetTimeFromPosition(int x);
        void SetValueFromPosition(int y, int trackIndex);
        void StartScrubbing();
        void StopScrubbing();
        bool IsScrubbing() const { return m_scrubbing; }

        // Callbacks
        void SetTimeChangedCallback(std::function<void(float)> callback);
        void SetKeyframeChangedCallback(std::function<void(int, int, float, float)> callback);

        // Getters
        int GetSelectedTrack() const { return m_selectedTrack; }
        const std::vector<int>& GetSelectedKeyframes() const { return m_selectedKeyframes; }
        bool IsTrackSelected(int trackIndex) const;
        bool IsKeyframeSelected(int trackIndex, int keyframeIndex) const;

    private:
        class ViewportRenderer* m_viewportRenderer;
        
        // Timeline state
        float m_currentTime;
        float m_duration;
        float m_fps;
        float m_zoom;
        PlaybackState m_playbackState;
        bool m_scrubbing;
        
        // Animation data
        std::vector<AnimationTrack> m_tracks;
        int m_selectedTrack;
        std::vector<int> m_selectedKeyframes;
        
        // UI state
        int m_trackHeight;
        int m_timeRulerHeight;
        int m_scrollOffset;
        int m_dragStartX;
        int m_dragStartY;
        bool m_dragging;
        int m_dragTrack;
        int m_dragKeyframe;
        
        // Callbacks
        std::function<void(float)> m_timeChangedCallback;
        std::function<void(int, int, float, float)> m_keyframeChangedCallback;
        
        // Helper methods
        void RenderTimeRuler(HDC hdc);
        void RenderTracks(HDC hdc);
        void RenderTrack(HDC hdc, int trackIndex);
        void RenderKeyframes(HDC hdc, int trackIndex);
        void RenderPlaybackControls(HDC hdc);
        void RenderTimeDisplay(HDC hdc);
        
        void UpdateTrackSelection(int y);
        void UpdateKeyframeSelection(int x, int y);
        void StartKeyframeDrag(int trackIndex, int keyframeIndex);
        void UpdateKeyframeDrag(int x, int y);
        void EndKeyframeDrag();
        
        float TimeFromPosition(int x) const;
        int PositionFromTime(float time) const;
        float ValueFromPosition(int y, int trackIndex) const;
        int PositionFromValue(float value, int trackIndex) const;
        
        void SnapToGrid(float& time);
        void SnapToKeyframe(float& time);
        
        // Animation helpers
        float LinearInterpolation(float a, float b, float t);
        float BezierInterpolation(float p0, float p1, float p2, float p3, float t);
        float EaseInOut(float t);
        float EaseIn(float t);
        float EaseOut(float t);
    };

} // namespace UI
} // namespace Engine
