#include "ui/TimelineSystem.h"
#include <iostream>
#include "ui/ViewportRenderer.h"
#include "core/Logger.h"
#include "core/Engine.h"
#include "core/AnimationManager.h"
#include <algorithm>
#include <cmath>

namespace Engine {
namespace UI {

    TimelineSystem::TimelineSystem()
        : UIComponent("TimelineSystem", 0, 0, 800, 150)
        , m_viewportRenderer(nullptr)
        , m_currentTime(0.0f)
        , m_duration(10.0f)
        , m_fps(30.0f)
        , m_zoom(1.0f)
        , m_playbackState(PlaybackState::Stopped)
        , m_scrubbing(false)
        , m_selectedTrack(-1)
        , m_trackHeight(25)
        , m_timeRulerHeight(30)
        , m_scrollOffset(0)
        , m_dragStartX(0)
        , m_dragStartY(0)
        , m_dragging(false)
        , m_dragTrack(-1)
        , m_dragKeyframe(-1)
    {
        Engine::Core::Logger::Instance().Debug("TimelineSystem created");
    }

    TimelineSystem::~TimelineSystem()
    {
        Shutdown();
        Engine::Core::Logger::Instance().Debug("TimelineSystem destroyed");
    }

    bool TimelineSystem::Initialize(class ViewportRenderer* viewportRenderer)
    {
        if (!viewportRenderer) {
            Engine::Core::Logger::Instance().Error("ViewportRenderer is null");
            return false;
        }

        m_viewportRenderer = viewportRenderer;
        
        // Add some default tracks for testing
        AddTrack("Position X", KeyframeType::Position, "DefaultObject", "PositionX");
        AddTrack("Position Y", KeyframeType::Position, "DefaultObject", "PositionY");
        AddTrack("Position Z", KeyframeType::Position, "DefaultObject", "PositionZ");
        AddTrack("Rotation X", KeyframeType::Rotation, "DefaultObject", "RotationX");
        AddTrack("Rotation Y", KeyframeType::Rotation, "DefaultObject", "RotationY");
        AddTrack("Rotation Z", KeyframeType::Rotation, "DefaultObject", "RotationZ");
        
        Engine::Core::Logger::Instance().Info("TimelineSystem initialized");
        return true;
    }

    void TimelineSystem::Shutdown()
    {
        m_viewportRenderer = nullptr;
        m_tracks.clear();
        m_selectedKeyframes.clear();
        Engine::Core::Logger::Instance().Info("TimelineSystem shutdown");
    }

    void TimelineSystem::Update(float deltaTime)
    {
        if (m_playbackState == PlaybackState::Playing) {
            m_currentTime += deltaTime;
            if (m_currentTime >= m_duration) {
                m_currentTime = m_duration;
                m_playbackState = PlaybackState::Stopped;
            }
            
            UpdateAnimation(deltaTime);
            
            if (m_timeChangedCallback) {
                m_timeChangedCallback(m_currentTime);
            }
        }
    }

    void TimelineSystem::Render(HDC hdc)
    {
        if (!m_visible) {
            return;
        }

        // Draw background
        HBRUSH hBrush = CreateSolidBrush(RGB(40, 40, 40));
        RECT rect = { m_x, m_y, m_x + m_width, m_y + m_height };
        FillRect(hdc, &rect, hBrush);
        DeleteObject(hBrush);

        // Draw border
        HPEN hPen = CreatePen(PS_SOLID, 1, RGB(80, 80, 80));
        HGDIOBJ hOldPen = ::SelectObject(hdc, hPen);
        MoveToEx(hdc, m_x, m_y, NULL);
        LineTo(hdc, m_x + m_width, m_y);
        LineTo(hdc, m_x + m_width, m_y + m_height);
        LineTo(hdc, m_x, m_y + m_height);
        LineTo(hdc, m_x, m_y);
        ::SelectObject(hdc, hOldPen);
        DeleteObject(hPen);

        // Draw title
        ::SetTextColor(hdc, RGB(255, 255, 255));
        SetBkMode(hdc, TRANSPARENT);
        RECT titleRect = { m_x + 5, m_y + 5, m_x + m_width - 5, m_y + 25 };
        DrawText(hdc, L"Timeline", -1, &titleRect, DT_LEFT | DT_VCENTER);

        // Render time ruler
        RenderTimeRuler(hdc);
        
        // Render tracks
        RenderTracks(hdc);
        
        // Render playback controls
        RenderPlaybackControls(hdc);
        
        // Render time display
        RenderTimeDisplay(hdc);
    }

    void TimelineSystem::HandleMouseEvent(int x, int y, int button, bool pressed)
    {
        // DEBUG: Log all clicks
        static int eventCount = 0;
        if (pressed && button == 0) {
            eventCount++;
            if (eventCount <= 10) {
                std::cout << "[TimelineSystem] HandleMouseEvent #" << eventCount << ": x=" << x << ", y=" << y 
                          << ", visible=" << m_visible << ", bounds: x=[" << m_x << "," << (m_x + m_width) 
                          << "], y=[" << m_y << "," << (m_y + m_height) << "]" << std::endl;
            }
        }
        
        if (!m_visible) {
            if (pressed && button == 0 && eventCount <= 5) {
                std::cout << "[TimelineSystem] Rejected: not visible" << std::endl;
            }
            return;
        }

        // Convert coordinates from parent-relative to component-relative
        int localX = x - m_x;
        int localY = y - m_y;
        
        // Check if click is within bounds (using local coordinates)
        if (localX < 0 || localX >= m_width || localY < 0 || localY >= m_height) {
            if (pressed && button == 0 && eventCount <= 5) {
                std::cout << "[TimelineSystem] Click outside bounds: localX=" << localX << " not in [0," << m_width 
                          << "], localY=" << localY << " not in [0," << m_height << "]" << std::endl;
            }
            return;
        }
        
        if (pressed && button == 0 && eventCount <= 10) {
            std::cout << "[TimelineSystem] Click ACCEPTED at (" << localX << "," << localY << ")" << std::endl;
        }

        if (button == 0) { // Left mouse button
            if (pressed) {
                m_dragStartX = localX;
                m_dragStartY = localY;
                
                std::cout << "[TimelineSystem] ========================================" << std::endl;
                std::cout << "[TimelineSystem] Click processing: localX=" << localX << ", localY=" << localY << std::endl;
                std::cout << "[TimelineSystem] Time ruler: Y=[" << m_timeRulerHeight << "," << (m_timeRulerHeight + 20) << "]" << std::endl;
                std::cout << "[TimelineSystem] Tracks start at Y=" << (m_timeRulerHeight + 20) << std::endl;
                std::cout << "[TimelineSystem] Total tracks: " << m_tracks.size() << std::endl;
                
                // Check if clicking on time ruler (using local coordinates)
                if (localY >= m_timeRulerHeight && localY <= m_timeRulerHeight + 20) {
                    std::cout << "[TimelineSystem] ✓ Clicked on TIME RULER at position " << localX << std::endl;
                    SetTimeFromPosition(localX);
                    StartScrubbing();
                    std::cout << "[TimelineSystem] Started scrubbing" << std::endl;
                    Engine::Core::Logger::Instance().Info("TimelineSystem: Started scrubbing at position " + std::to_string(localX));
                }
                // Check if clicking on tracks (using local coordinates)
                else if (localY >= m_timeRulerHeight + 20) {
                    int trackAreaY = localY - (m_timeRulerHeight + 20);
                    std::cout << "[TimelineSystem] ✓ Clicked on TRACK AREA at (" << localX << "," << localY 
                              << "), trackAreaY=" << trackAreaY << std::endl;
                    std::cout << "[TimelineSystem] Calling UpdateTrackSelection(" << localY << ")" << std::endl;
                    UpdateTrackSelection(localY);
                    std::cout << "[TimelineSystem] Calling UpdateKeyframeSelection(" << localX << "," << localY << ")" << std::endl;
                    UpdateKeyframeSelection(localX, localY);
                    Engine::Core::Logger::Instance().Info("TimelineSystem: Updated track/keyframe selection");
                } else {
                    std::cout << "[TimelineSystem] ✗ Click is above time ruler (localY=" << localY 
                              << " < " << m_timeRulerHeight << ")" << std::endl;
                }
                std::cout << "[TimelineSystem] ========================================" << std::endl;
            } else {
                if (m_dragging) {
                    EndKeyframeDrag();
                }
                if (m_scrubbing) {
                    StopScrubbing();
                }
            }
        }
    }

    void TimelineSystem::HandleKeyboardEvent(UINT message, WPARAM wParam, LPARAM lParam)
    {
        (void)lParam; // Suppress unused parameter warning
        
        if (!m_visible) {
            return;
        }

        if (message == WM_KEYDOWN) {
            switch (wParam) {
                case VK_SPACE:
                    if (m_playbackState == PlaybackState::Playing) {
                        Pause();
                    } else {
                        Play();
                    }
                    break;
                    
                case VK_HOME:
                    SetCurrentTime(0.0f);
                    break;
                    
                case VK_END:
                    SetCurrentTime(m_duration);
                    break;
                    
                case VK_LEFT:
                    SetCurrentTime(m_currentTime - 1.0f / m_fps);
                    break;
                    
                case VK_RIGHT:
                    SetCurrentTime(m_currentTime + 1.0f / m_fps);
                    break;
                    
                case VK_DELETE:
                    // Delete selected keyframes
                    for (int i = static_cast<int>(m_selectedKeyframes.size()) - 1; i >= 0; --i) {
                        if (m_selectedTrack >= 0 && m_selectedTrack < static_cast<int>(m_tracks.size())) {
                            int keyframeIndex = m_selectedKeyframes[i];
                            if (keyframeIndex >= 0 && keyframeIndex < static_cast<int>(m_tracks[m_selectedTrack].keyframes.size())) {
                                RemoveKeyframe(m_selectedTrack, keyframeIndex);
                            }
                        }
                    }
                    m_selectedKeyframes.clear();
                    break;
            }
        }
    }

    void TimelineSystem::Play()
    {
        m_playbackState = PlaybackState::Playing;
        Engine::Core::Logger::Instance().Info("Timeline: Play");
        
        // Wire to AnimationManager
        extern Engine::Core::Engine* g_engine;
        if (g_engine && g_engine->GetAnimationManager()) {
            // TODO: Start playing active animation clip
            // For now, just log the action
            Engine::Core::Logger::Instance().Debug("AnimationManager: Play triggered from Timeline");
        }
    }

    void TimelineSystem::Pause()
    {
        m_playbackState = PlaybackState::Paused;
        Engine::Core::Logger::Instance().Info("Timeline: Pause");
        
        // Wire to AnimationManager
        extern Engine::Core::Engine* g_engine;
        if (g_engine && g_engine->GetAnimationManager()) {
            // TODO: Pause active animation clip
            Engine::Core::Logger::Instance().Debug("AnimationManager: Pause triggered from Timeline");
        }
    }

    void TimelineSystem::Stop()
    {
        m_playbackState = PlaybackState::Stopped;
        m_currentTime = 0.0f;
        Engine::Core::Logger::Instance().Info("Timeline: Stop");
        
        // Wire to AnimationManager
        extern Engine::Core::Engine* g_engine;
        if (g_engine && g_engine->GetAnimationManager()) {
            // TODO: Stop active animation clip and reset to start
            Engine::Core::Logger::Instance().Debug("AnimationManager: Stop triggered from Timeline");
        }
    }

    void TimelineSystem::SetCurrentTime(float time)
    {
        m_currentTime = std::max(0.0f, std::min(m_duration, time));
        EvaluateAnimation(m_currentTime);
        
        if (m_timeChangedCallback) {
            m_timeChangedCallback(m_currentTime);
        }
    }

    void TimelineSystem::SetDuration(float duration)
    {
        m_duration = std::max(1.0f, duration);
        if (m_currentTime > m_duration) {
            m_currentTime = m_duration;
        }
    }

    void TimelineSystem::SetFPS(float fps)
    {
        m_fps = std::max(1.0f, fps);
    }

    void TimelineSystem::SetZoom(float zoom)
    {
        m_zoom = std::max(0.1f, std::min(10.0f, zoom));
    }

    void TimelineSystem::AddTrack(const std::string& name, KeyframeType type, const std::string& objectName, const std::string& propertyName)
    {
        m_tracks.emplace_back(name, type, objectName, propertyName);
        Engine::Core::Logger::Instance().Info("Added track: " + name);
    }

    void TimelineSystem::RemoveTrack(int trackIndex)
    {
        if (trackIndex >= 0 && trackIndex < static_cast<int>(m_tracks.size())) {
            m_tracks.erase(m_tracks.begin() + trackIndex);
            if (m_selectedTrack == trackIndex) {
                m_selectedTrack = -1;
            }
            Engine::Core::Logger::Instance().Info("Removed track at index " + std::to_string(trackIndex));
        }
    }

    void TimelineSystem::SetTrackVisible(int trackIndex, bool visible)
    {
        if (trackIndex >= 0 && trackIndex < static_cast<int>(m_tracks.size())) {
            m_tracks[trackIndex].visible = visible;
        }
    }

    void TimelineSystem::SetTrackLocked(int trackIndex, bool locked)
    {
        if (trackIndex >= 0 && trackIndex < static_cast<int>(m_tracks.size())) {
            m_tracks[trackIndex].locked = locked;
        }
    }

    void TimelineSystem::AddKeyframe(int trackIndex, float time, float value)
    {
        if (trackIndex >= 0 && trackIndex < static_cast<int>(m_tracks.size())) {
            time = std::max(0.0f, std::min(m_duration, time));
            value = std::max(m_tracks[trackIndex].minValue, std::min(m_tracks[trackIndex].maxValue, value));
            
            m_tracks[trackIndex].keyframes.emplace_back(time, value, m_tracks[trackIndex].type, m_tracks[trackIndex].objectName, m_tracks[trackIndex].propertyName);
            
            // Sort keyframes by time
            std::sort(m_tracks[trackIndex].keyframes.begin(), m_tracks[trackIndex].keyframes.end(),
                [](const Keyframe& a, const Keyframe& b) { return a.time < b.time; });
            
            Engine::Core::Logger::Instance().Info("Added keyframe at time " + std::to_string(time) + " with value " + std::to_string(value));
        }
    }

    void TimelineSystem::RemoveKeyframe(int trackIndex, int keyframeIndex)
    {
        if (trackIndex >= 0 && trackIndex < static_cast<int>(m_tracks.size()) &&
            keyframeIndex >= 0 && keyframeIndex < static_cast<int>(m_tracks[trackIndex].keyframes.size())) {
            m_tracks[trackIndex].keyframes.erase(m_tracks[trackIndex].keyframes.begin() + keyframeIndex);
            Engine::Core::Logger::Instance().Info("Removed keyframe at index " + std::to_string(keyframeIndex));
        }
    }

    void TimelineSystem::MoveKeyframe(int trackIndex, int keyframeIndex, float newTime, float newValue)
    {
        if (trackIndex >= 0 && trackIndex < static_cast<int>(m_tracks.size()) &&
            keyframeIndex >= 0 && keyframeIndex < static_cast<int>(m_tracks[trackIndex].keyframes.size())) {
            
            newTime = std::max(0.0f, std::min(m_duration, newTime));
            newValue = std::max(m_tracks[trackIndex].minValue, std::min(m_tracks[trackIndex].maxValue, newValue));
            
            m_tracks[trackIndex].keyframes[keyframeIndex].time = newTime;
            m_tracks[trackIndex].keyframes[keyframeIndex].value = newValue;
            
            // Re-sort keyframes
            std::sort(m_tracks[trackIndex].keyframes.begin(), m_tracks[trackIndex].keyframes.end(),
                [](const Keyframe& a, const Keyframe& b) { return a.time < b.time; });
            
            if (m_keyframeChangedCallback) {
                m_keyframeChangedCallback(trackIndex, keyframeIndex, newTime, newValue);
            }
        }
    }

    void TimelineSystem::SelectKeyframe(int trackIndex, int keyframeIndex, bool multiSelect)
    {
        if (!multiSelect) {
            m_selectedKeyframes.clear();
        }
        
        if (trackIndex >= 0 && trackIndex < static_cast<int>(m_tracks.size()) &&
            keyframeIndex >= 0 && keyframeIndex < static_cast<int>(m_tracks[trackIndex].keyframes.size())) {
            
            m_selectedTrack = trackIndex;
            m_selectedKeyframes.push_back(keyframeIndex);
            m_tracks[trackIndex].keyframes[keyframeIndex].selected = true;
        }
    }

    void TimelineSystem::ClearSelection()
    {
        for (auto& track : m_tracks) {
            for (auto& keyframe : track.keyframes) {
                keyframe.selected = false;
            }
        }
        m_selectedKeyframes.clear();
        m_selectedTrack = -1;
    }

    void TimelineSystem::UpdateAnimation(float deltaTime)
    {
        (void)deltaTime; // Suppress unused parameter warning
        EvaluateAnimation(m_currentTime);
    }

    void TimelineSystem::EvaluateAnimation(float time)
    {
        for (auto& track : m_tracks) {
            if (track.keyframes.empty()) {
                continue;
            }
            
            float value;
            InterpolateKeyframes(track, time, value);
            
            // TODO: Apply value to actual object property
            // This would involve calling the scene manager to update object properties
        }
    }

    void TimelineSystem::InterpolateKeyframes(const AnimationTrack& track, float time, float& result)
    {
        if (track.keyframes.empty()) {
            result = 0.0f;
            return;
        }
        
        if (track.keyframes.size() == 1) {
            result = track.keyframes[0].value;
            return;
        }
        
        // Find surrounding keyframes
        int beforeIndex = -1;
        int afterIndex = -1;
        
        for (size_t i = 0; i < track.keyframes.size(); ++i) {
            if (track.keyframes[i].time <= time) {
                beforeIndex = static_cast<int>(i);
            }
            if (track.keyframes[i].time >= time && afterIndex == -1) {
                afterIndex = static_cast<int>(i);
                break;
            }
        }
        
        if (beforeIndex == -1) {
            result = track.keyframes[0].value;
        } else if (afterIndex == -1) {
            result = track.keyframes.back().value;
        } else if (beforeIndex == afterIndex) {
            result = track.keyframes[beforeIndex].value;
        } else {
            // Linear interpolation
            const auto& before = track.keyframes[beforeIndex];
            const auto& after = track.keyframes[afterIndex];
            
            float t = (time - before.time) / (after.time - before.time);
            result = LinearInterpolation(before.value, after.value, t);
        }
    }

    void TimelineSystem::SetTimeFromPosition(int x)
    {
        float time = TimeFromPosition(x);
        SetCurrentTime(time);
    }

    void TimelineSystem::SetValueFromPosition(int y, int trackIndex)
    {
        if (trackIndex >= 0 && trackIndex < static_cast<int>(m_tracks.size())) {
            float value = ValueFromPosition(y, trackIndex);
            (void)value; // Suppress unused variable warning
            // TODO: Set value to current time
        }
    }

    void TimelineSystem::StartScrubbing()
    {
        m_scrubbing = true;
    }

    void TimelineSystem::StopScrubbing()
    {
        m_scrubbing = false;
    }

    void TimelineSystem::SetTimeChangedCallback(std::function<void(float)> callback)
    {
        m_timeChangedCallback = callback;
    }

    void TimelineSystem::SetKeyframeChangedCallback(std::function<void(int, int, float, float)> callback)
    {
        m_keyframeChangedCallback = callback;
    }

    bool TimelineSystem::IsTrackSelected(int trackIndex) const
    {
        return trackIndex == m_selectedTrack;
    }

    bool TimelineSystem::IsKeyframeSelected(int trackIndex, int keyframeIndex) const
    {
        if (trackIndex != m_selectedTrack) {
            return false;
        }
        return std::find(m_selectedKeyframes.begin(), m_selectedKeyframes.end(), keyframeIndex) != m_selectedKeyframes.end();
    }

    void TimelineSystem::RenderTimeRuler(HDC hdc)
    {
        int rulerY = m_y + m_timeRulerHeight;
        int rulerHeight = 20;
        
        // Draw ruler background
        HBRUSH hBrush = CreateSolidBrush(RGB(50, 50, 50));
        RECT rulerRect = { m_x, rulerY, m_x + m_width, rulerY + rulerHeight };
        FillRect(hdc, &rulerRect, hBrush);
        DeleteObject(hBrush);
        
        // Draw time markers
        ::SetTextColor(hdc, RGB(200, 200, 200));
        SetBkMode(hdc, TRANSPARENT);
        
        float timeStep = 1.0f / m_zoom;
        float startTime = static_cast<float>(m_scrollOffset) / m_zoom;
        
        for (float time = startTime; time <= m_duration; time += timeStep) {
            int x = PositionFromTime(time);
            if (x < m_x || x > m_x + m_width) continue;
            
            // Draw vertical line
            HPEN hPen = CreatePen(PS_SOLID, 1, RGB(100, 100, 100));
            HGDIOBJ hOldPen = ::SelectObject(hdc, hPen);
            MoveToEx(hdc, x, rulerY, NULL);
            LineTo(hdc, x, rulerY + rulerHeight);
            ::SelectObject(hdc, hOldPen);
            DeleteObject(hPen);
            
            // Draw time label
            std::string timeStr = std::to_string(static_cast<int>(time)) + "s";
            std::wstring wideTimeStr(timeStr.begin(), timeStr.end());
            RECT textRect = { x + 2, rulerY, x + 50, rulerY + rulerHeight };
            DrawText(hdc, wideTimeStr.c_str(), -1, &textRect, DT_LEFT | DT_VCENTER);
        }
        
        // Draw current time indicator
        int currentX = PositionFromTime(m_currentTime);
        if (currentX >= m_x && currentX <= m_x + m_width) {
            HPEN hPen = CreatePen(PS_SOLID, 2, RGB(255, 100, 100));
            HGDIOBJ hOldPen = ::SelectObject(hdc, hPen);
            MoveToEx(hdc, currentX, rulerY, NULL);
            LineTo(hdc, currentX, rulerY + rulerHeight);
            ::SelectObject(hdc, hOldPen);
            DeleteObject(hPen);
        }
    }

    void TimelineSystem::RenderTracks(HDC hdc)
    {
        int trackY = m_y + m_timeRulerHeight + 20;
        (void)trackY; // Suppress unused variable warning
        
        for (size_t i = 0; i < m_tracks.size(); ++i) {
            if (!m_tracks[i].visible) continue;
            
            RenderTrack(hdc, static_cast<int>(i));
        }
    }

    void TimelineSystem::RenderTrack(HDC hdc, int trackIndex)
    {
        if (trackIndex < 0 || trackIndex >= static_cast<int>(m_tracks.size())) {
            return;
        }
        
        const auto& track = m_tracks[trackIndex];
        int trackY = m_y + m_timeRulerHeight + 20 + trackIndex * m_trackHeight;
        
        // Draw track background
        HBRUSH hBrush = CreateSolidBrush(IsTrackSelected(trackIndex) ? RGB(60, 80, 100) : RGB(45, 45, 45));
        RECT trackRect = { m_x, trackY, m_x + m_width, trackY + m_trackHeight };
        FillRect(hdc, &trackRect, hBrush);
        DeleteObject(hBrush);
        
        // Draw track border
        HPEN hPen = CreatePen(PS_SOLID, 1, RGB(70, 70, 70));
        HGDIOBJ hOldPen = ::SelectObject(hdc, hPen);
        MoveToEx(hdc, m_x, trackY, NULL);
        LineTo(hdc, m_x + m_width, trackY);
        LineTo(hdc, m_x + m_width, trackY + m_trackHeight);
        LineTo(hdc, m_x, trackY + m_trackHeight);
        LineTo(hdc, m_x, trackY);
        ::SelectObject(hdc, hOldPen);
        DeleteObject(hPen);
        
        // Draw track name
        ::SetTextColor(hdc, RGB(255, 255, 255));
        SetBkMode(hdc, TRANSPARENT);
        RECT nameRect = { m_x + 5, trackY, m_x + 150, trackY + m_trackHeight };
        std::wstring wideName(track.name.begin(), track.name.end());
        DrawText(hdc, wideName.c_str(), -1, &nameRect, DT_LEFT | DT_VCENTER);
        
        // Render keyframes
        RenderKeyframes(hdc, trackIndex);
    }

    void TimelineSystem::RenderKeyframes(HDC hdc, int trackIndex)
    {
        if (trackIndex < 0 || trackIndex >= static_cast<int>(m_tracks.size())) {
            return;
        }
        
        const auto& track = m_tracks[trackIndex];
        int trackY = m_y + m_timeRulerHeight + 20 + trackIndex * m_trackHeight;
        
        for (size_t i = 0; i < track.keyframes.size(); ++i) {
            const auto& keyframe = track.keyframes[i];
            int x = PositionFromTime(keyframe.time);
            int y = trackY + m_trackHeight / 2;
            
            if (x < m_x || x > m_x + m_width) continue;
            
            // Draw keyframe
            HBRUSH hBrush = CreateSolidBrush(keyframe.selected ? RGB(255, 200, 100) : RGB(100, 150, 255));
            RECT keyRect = { x - 4, y - 4, x + 4, y + 4 };
            FillRect(hdc, &keyRect, hBrush);
            DeleteObject(hBrush);
            
            // Draw keyframe border
            HPEN hPen = CreatePen(PS_SOLID, 1, RGB(255, 255, 255));
            HGDIOBJ hOldPen = ::SelectObject(hdc, hPen);
            MoveToEx(hdc, x - 4, y - 4, NULL);
            LineTo(hdc, x + 4, y - 4);
            LineTo(hdc, x + 4, y + 4);
            LineTo(hdc, x - 4, y + 4);
            LineTo(hdc, x - 4, y - 4);
            ::SelectObject(hdc, hOldPen);
            DeleteObject(hPen);
        }
    }

    void TimelineSystem::RenderPlaybackControls(HDC hdc)
    {
        int controlY = m_y + m_height - 30;
        int controlHeight = 25;
        
        // Draw control background
        HBRUSH hBrush = CreateSolidBrush(RGB(50, 50, 50));
        RECT controlRect = { m_x, controlY, m_x + m_width, controlY + controlHeight };
        FillRect(hdc, &controlRect, hBrush);
        DeleteObject(hBrush);
        
        // Draw play/pause button
        RECT buttonRect = { m_x + 10, controlY + 5, m_x + 40, controlY + controlHeight - 5 };
        hBrush = CreateSolidBrush(RGB(80, 120, 80));
        FillRect(hdc, &buttonRect, hBrush);
        DeleteObject(hBrush);
        
        // Draw button text
        ::SetTextColor(hdc, RGB(255, 255, 255));
        SetBkMode(hdc, TRANSPARENT);
        std::wstring buttonText = (m_playbackState == PlaybackState::Playing) ? L"Pause" : L"Play";
        DrawText(hdc, buttonText.c_str(), -1, &buttonRect, DT_CENTER | DT_VCENTER);
        
        // Draw stop button
        buttonRect = { m_x + 50, controlY + 5, m_x + 80, controlY + controlHeight - 5 };
        hBrush = CreateSolidBrush(RGB(120, 80, 80));
        FillRect(hdc, &buttonRect, hBrush);
        DeleteObject(hBrush);
        
        ::SetTextColor(hdc, RGB(255, 255, 255));
        DrawText(hdc, L"Stop", -1, &buttonRect, DT_CENTER | DT_VCENTER);
    }

    void TimelineSystem::RenderTimeDisplay(HDC hdc)
    {
        int displayY = m_y + m_height - 30;
        int displayHeight = 25;
        
        // Draw time display
        std::string timeStr = "Time: " + std::to_string(static_cast<int>(m_currentTime)) + "s / " + std::to_string(static_cast<int>(m_duration)) + "s";
        std::wstring wideTimeStr(timeStr.begin(), timeStr.end());
        
        ::SetTextColor(hdc, RGB(255, 255, 255));
        SetBkMode(hdc, TRANSPARENT);
        RECT timeRect = { m_x + m_width - 150, displayY, m_x + m_width - 10, displayY + displayHeight };
        DrawText(hdc, wideTimeStr.c_str(), -1, &timeRect, DT_RIGHT | DT_VCENTER);
    }

    void TimelineSystem::UpdateTrackSelection(int y)
    {
        std::cout << "[TimelineSystem] UpdateTrackSelection: y=" << y << " (absolute)" << std::endl;
        std::cout << "[TimelineSystem]   m_y=" << m_y << ", m_timeRulerHeight=" << m_timeRulerHeight 
                  << ", trackHeight=" << m_trackHeight << std::endl;
        int trackAreaY = y - m_y - m_timeRulerHeight - 20;
        std::cout << "[TimelineSystem]   trackAreaY=" << trackAreaY << std::endl;
        int trackIndex = trackAreaY / m_trackHeight;
        std::cout << "[TimelineSystem]   Calculated trackIndex=" << trackIndex << " (tracks=" << m_tracks.size() << ")" << std::endl;
        if (trackIndex >= 0 && trackIndex < static_cast<int>(m_tracks.size())) {
            m_selectedTrack = trackIndex;
            std::cout << "[TimelineSystem] ✓ Selected track #" << trackIndex << ": '" << m_tracks[trackIndex].name << "'" << std::endl;
        } else {
            std::cout << "[TimelineSystem] ✗ Invalid track index: " << trackIndex << std::endl;
        }
    }

    void TimelineSystem::UpdateKeyframeSelection(int x, int y)
    {
        std::cout << "[TimelineSystem] UpdateKeyframeSelection: x=" << x << ", y=" << y << " (absolute)" << std::endl;
        int trackAreaY = y - m_y - m_timeRulerHeight - 20;
        int trackIndex = trackAreaY / m_trackHeight;
        std::cout << "[TimelineSystem]   Calculated trackIndex=" << trackIndex << " (tracks=" << m_tracks.size() << ")" << std::endl;
        
        if (trackIndex < 0 || trackIndex >= static_cast<int>(m_tracks.size())) {
            std::cout << "[TimelineSystem] ✗ Invalid track index: " << trackIndex << std::endl;
            return;
        }
        
        const auto& track = m_tracks[trackIndex];
        std::cout << "[TimelineSystem]   Checking track #" << trackIndex << ": '" << track.name 
                  << "' (keyframes=" << track.keyframes.size() << ")" << std::endl;
        
        for (size_t i = 0; i < track.keyframes.size(); ++i) {
            int keyX = PositionFromTime(track.keyframes[i].time);
            int distance = abs(x - keyX);
            std::cout << "[TimelineSystem]     Keyframe #" << i << ": time=" << track.keyframes[i].time 
                      << ", keyX=" << keyX << ", distance=" << distance << std::endl;
            if (distance <= 4) {
                bool multiSelect = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
                std::cout << "[TimelineSystem] ✓ Selected keyframe #" << i << " (multiSelect=" << multiSelect << ")" << std::endl;
                SelectKeyframe(trackIndex, static_cast<int>(i), multiSelect);
                break;
            }
        }
    }

    void TimelineSystem::StartKeyframeDrag(int trackIndex, int keyframeIndex)
    {
        m_dragging = true;
        m_dragTrack = trackIndex;
        m_dragKeyframe = keyframeIndex;
    }

    void TimelineSystem::UpdateKeyframeDrag(int x, int y)
    {
        if (m_dragging && m_dragTrack >= 0 && m_dragKeyframe >= 0) {
            float newTime = TimeFromPosition(x);
            float newValue = ValueFromPosition(y, m_dragTrack);
            MoveKeyframe(m_dragTrack, m_dragKeyframe, newTime, newValue);
        }
    }

    void TimelineSystem::EndKeyframeDrag()
    {
        m_dragging = false;
        m_dragTrack = -1;
        m_dragKeyframe = -1;
    }

    float TimelineSystem::TimeFromPosition(int x) const
    {
        float relativeX = static_cast<float>(x - m_x - 50);
        return relativeX / m_zoom;
    }

    int TimelineSystem::PositionFromTime(float time) const
    {
        return m_x + 50 + static_cast<int>(time * m_zoom);
    }

    float TimelineSystem::ValueFromPosition(int y, int trackIndex) const
    {
        if (trackIndex < 0 || trackIndex >= static_cast<int>(m_tracks.size())) {
            return 0.0f;
        }
        
        const auto& track = m_tracks[trackIndex];
        int trackY = m_y + m_timeRulerHeight + 20 + trackIndex * m_trackHeight;
        float relativeY = static_cast<float>(y - trackY - m_trackHeight / 2);
        float normalizedY = relativeY / (m_trackHeight / 2.0f);
        return track.minValue + (track.maxValue - track.minValue) * (1.0f - normalizedY) * 0.5f;
    }

    int TimelineSystem::PositionFromValue(float value, int trackIndex) const
    {
        if (trackIndex < 0 || trackIndex >= static_cast<int>(m_tracks.size())) {
            return 0;
        }
        
        const auto& track = m_tracks[trackIndex];
        float normalizedValue = (value - track.minValue) / (track.maxValue - track.minValue);
        int trackY = m_y + m_timeRulerHeight + 20 + trackIndex * m_trackHeight;
        return trackY + m_trackHeight / 2 - static_cast<int>(normalizedValue * m_trackHeight / 2.0f);
    }

    void TimelineSystem::SnapToGrid(float& time)
    {
        float gridSize = 1.0f / m_fps;
        time = std::round(time / gridSize) * gridSize;
    }

    void TimelineSystem::SnapToKeyframe(float& time)
    {
        float snapThreshold = 0.1f;
        for (const auto& track : m_tracks) {
            for (const auto& keyframe : track.keyframes) {
                if (abs(keyframe.time - time) < snapThreshold) {
                    time = keyframe.time;
                    return;
                }
            }
        }
    }

    float TimelineSystem::LinearInterpolation(float a, float b, float t)
    {
        return a + (b - a) * t;
    }

    float TimelineSystem::BezierInterpolation(float p0, float p1, float p2, float p3, float t)
    {
        float u = 1.0f - t;
        float tt = t * t;
        float uu = u * u;
        float uuu = uu * u;
        float ttt = tt * t;
        
        return uuu * p0 + 3 * uu * t * p1 + 3 * u * tt * p2 + ttt * p3;
    }

    float TimelineSystem::EaseInOut(float t)
    {
        return t < 0.5f ? 2.0f * t * t : -1.0f + (4.0f - 2.0f * t) * t;
    }

    float TimelineSystem::EaseIn(float t)
    {
        return t * t;
    }

    float TimelineSystem::EaseOut(float t)
    {
        return t * (2.0f - t);
    }

} // namespace UI
} // namespace Engine
