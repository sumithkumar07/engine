#include "ui/AssetBrowser.h"
#include "core/Logger.h"
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <shlobj.h>
#include <commdlg.h>
#include <fstream>
#include <ctime>
#include <iostream>

namespace Engine {
namespace UI {

    AssetBrowser::AssetBrowser()
        : UIComponent("AssetBrowser", 0, 0, 300, 400)
        , m_listView(true)
        , m_thumbnailSize(DEFAULT_THUMBNAIL_SIZE)
        , m_showHiddenFiles(false)
        , m_scrollOffset(0)
        , m_itemHeight(20)
        , m_thumbnailPadding(4)
        , m_selectedIndex(-1)
        , m_multiSelect(false)
        , m_dragging(false)
        , m_dragStartX(0)
        , m_dragStartY(0)
        , m_filterType(AssetType::Unknown)
        , m_searchQuery("")
        , m_filterExtension("")
    {
        Engine::Core::Logger::Instance().Debug("AssetBrowser created");
    }

    AssetBrowser::~AssetBrowser()
    {
        Shutdown();
        Engine::Core::Logger::Instance().Debug("AssetBrowser destroyed");
    }

    bool AssetBrowser::Initialize(const std::string& rootPath)
    {
        std::cout << "DEBUG: AssetBrowser::Initialize() START" << std::endl;
        Engine::Core::Logger::Instance().Debug("AssetBrowser::Initialize() START");
        
        if (rootPath.empty()) {
            std::cout << "DEBUG: rootPath is empty, using default" << std::endl;
            // Get default documents folder
            char documentsPath[MAX_PATH];
            if (SUCCEEDED(SHGetFolderPathA(NULL, CSIDL_MYDOCUMENTS, NULL, SHGFP_TYPE_CURRENT, documentsPath))) {
                m_rootPath = std::string(documentsPath) + "\\AI_Movie_Studio\\Assets";
            } else {
                m_rootPath = "C:\\AI_Movie_Studio\\Assets";
            }
        } else {
            std::cout << "DEBUG: Using provided rootPath: " << rootPath << std::endl;
            m_rootPath = rootPath;
        }

        std::cout << "DEBUG: Setting m_rootPath to: " << m_rootPath << std::endl;
        Engine::Core::Logger::Instance().Debug("Setting m_rootPath to: " + m_rootPath);

        // Create root directory if it doesn't exist
        std::cout << "DEBUG: Creating directories" << std::endl;
        std::filesystem::create_directories(m_rootPath);
        
        m_currentPath = m_rootPath;
        std::cout << "DEBUG: Set m_currentPath to: " << m_currentPath << std::endl;
        
        // Build file tree
        std::cout << "DEBUG: Calling BuildFileTree()" << std::endl;
        BuildFileTree();
        std::cout << "DEBUG: BuildFileTree() completed" << std::endl;
        
        // Refresh current directory
        std::cout << "DEBUG: Calling RefreshCurrentDirectory()" << std::endl;
        RefreshCurrentDirectory();
        std::cout << "DEBUG: RefreshCurrentDirectory() completed" << std::endl;
        
        // Add sample assets for demonstration
        std::cout << "DEBUG: About to call AddSampleAssets()" << std::endl;
        Engine::Core::Logger::Instance().Debug("About to call AddSampleAssets()");
        
        try {
            AddSampleAssets();
            std::cout << "DEBUG: AddSampleAssets() call completed successfully" << std::endl;
            Engine::Core::Logger::Instance().Debug("AddSampleAssets() call completed successfully");
        } catch (const std::exception& e) {
            std::cout << "DEBUG: Exception in AddSampleAssets(): " << e.what() << std::endl;
            Engine::Core::Logger::Instance().Error("Exception in AddSampleAssets(): " + std::string(e.what()));
        } catch (...) {
            std::cout << "DEBUG: Unknown exception in AddSampleAssets()" << std::endl;
            Engine::Core::Logger::Instance().Error("Unknown exception in AddSampleAssets()");
        }
        
        std::cout << "DEBUG: AssetBrowser::Initialize() END" << std::endl;
        Engine::Core::Logger::Instance().Info("AssetBrowser initialized with root path: " + m_rootPath);
        return true;
    }

    void AssetBrowser::Shutdown()
    {
        // Clean up thumbnails
        for (auto& asset : m_currentAssets) {
            if (asset->thumbnail) {
                DeleteObject(asset->thumbnail);
                asset->thumbnail = nullptr;
            }
        }
        
        m_currentAssets.clear();
        m_filteredAssets.clear();
        m_selectedAssets.clear();
        m_rootNode.reset();
        
        Engine::Core::Logger::Instance().Info("AssetBrowser shutdown");
    }

    void AssetBrowser::Update(float deltaTime)
    {
        (void)deltaTime; // Suppress unused parameter warning
        
        // Update scrollbar if needed
        UpdateScrollbar();
    }

    void AssetBrowser::Render(HDC hdc)
    {
        if (!m_visible) {
            return;
        }

        // Draw background
        HBRUSH hBrush = CreateSolidBrush(RGB(45, 45, 45));
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
        DrawText(hdc, L"Asset Browser", -1, &titleRect, DT_LEFT | DT_VCENTER);

        // Render toolbar
        RenderToolbar(hdc);
        
        // Render search bar
        RenderSearchBar(hdc);
        
        // Render folder tree (left side)
        int treeWidth = m_width / 3;
        RECT treeRect = { m_x + 5, m_y + 60, m_x + treeWidth, m_y + m_height - 5 };
        FillRect(hdc, &treeRect, CreateSolidBrush(RGB(40, 40, 40)));
        
        // Render asset list (right side)
        RECT assetRect = { m_x + treeWidth + 10, m_y + 60, m_x + m_width - 5, m_y + m_height - 5 };
        FillRect(hdc, &assetRect, CreateSolidBrush(RGB(50, 50, 50)));
        
        // Render folder tree
        int treeY = m_y + 65;
        (void)treeY; // Suppress unused variable warning
        RenderFolderTree(hdc);
        
        // Render asset list
        RenderAssetList(hdc);
    }

    void AssetBrowser::HandleMouseEvent(int x, int y, int button, bool pressed)
    {
        // DEBUG: Log all clicks
        static int eventCount = 0;
        if (pressed && button == 0) {
            eventCount++;
            if (eventCount <= 10) {
                std::cout << "[AssetBrowser] HandleMouseEvent #" << eventCount << ": x=" << x << ", y=" << y 
                          << ", visible=" << m_visible << ", bounds: x=[" << m_x << "," << (m_x + m_width) 
                          << "], y=[" << m_y << "," << (m_y + m_height) << "]" << std::endl;
            }
        }
        
        if (!m_visible) {
            if (pressed && button == 0 && eventCount <= 5) {
                std::cout << "[AssetBrowser] Rejected: not visible" << std::endl;
            }
            return;
        }

        // Convert coordinates from parent-relative to component-relative
        int localX = x - m_x;
        int localY = y - m_y;
        
        // Check if click is within bounds (using local coordinates)
        if (localX < 0 || localX >= m_width || localY < 0 || localY >= m_height) {
            if (pressed && button == 0 && eventCount <= 5) {
                std::cout << "[AssetBrowser] Click outside bounds: localX=" << localX << " not in [0," << m_width 
                          << "], localY=" << localY << " not in [0," << m_height << "]" << std::endl;
            }
            return;
        }
        
        if (pressed && button == 0 && eventCount <= 10) {
            std::cout << "[AssetBrowser] Click ACCEPTED at (" << localX << "," << localY << ")" << std::endl;
        }

        if (button == 0) { // Left mouse button
            if (pressed) {
                m_dragStartX = localX;
                m_dragStartY = localY;
                
                // Check if clicking on folder tree (using local coordinates)
                int treeWidth = m_width / 3;
                if (localX >= 5 && localX <= treeWidth) {
                    HandleFolderClick(localX, localY);
                }
                // Check if clicking on asset list
                else if (localX >= treeWidth + 10) {
                    HandleAssetClick(localX, localY);
                }
            } else {
                if (m_dragging) {
                    m_dragging = false;
                    m_dragAssetPath.clear();
                }
            }
        } else if (button == 1) { // Right mouse button
            if (pressed) {
                HandleRightClick(x, y);
            }
        } else if (button == 0 && !pressed) { // Left mouse button release (double click)
            static DWORD lastClickTime = 0;
            DWORD currentTime = GetTickCount();
            if (currentTime - lastClickTime < 500) { // Double click within 500ms
                HandleAssetDoubleClick(x, y);
            }
            lastClickTime = currentTime;
        }
    }

    void AssetBrowser::HandleKeyboardEvent(UINT message, WPARAM wParam, LPARAM lParam)
    {
        (void)lParam; // Suppress unused parameter warning
        
        if (!m_visible) {
            return;
        }

        if (message == WM_KEYDOWN) {
            switch (wParam) {
                case VK_UP:
                    if (m_selectedIndex > 0) {
                        SelectAsset(m_selectedIndex - 1);
                    }
                    break;
                    
                case VK_DOWN:
                    if (m_selectedIndex < static_cast<int>(m_filteredAssets.size()) - 1) {
                        SelectAsset(m_selectedIndex + 1);
                    }
                    break;
                    
                case VK_RETURN:
                    if (m_selectedIndex >= 0 && m_selectedIndex < static_cast<int>(m_filteredAssets.size())) {
                        auto asset = m_filteredAssets[m_selectedIndex];
                        if (asset->isDirectory) {
                            NavigateToPath(asset->path);
                        } else if (m_assetDoubleClickedCallback) {
                            m_assetDoubleClickedCallback(asset->path);
                        }
                    }
                    break;
                    
                case VK_BACK:
                    NavigateUp();
                    break;
                    
                case VK_DELETE:
                    if (m_selectedIndex >= 0 && m_selectedIndex < static_cast<int>(m_filteredAssets.size())) {
                        auto asset = m_filteredAssets[m_selectedIndex];
                        if (!asset->isDirectory) {
                            DeleteAsset(asset->path);
                        }
                    }
                    break;
                    
                case VK_F5:
                    Refresh();
                    break;
            }
        }
    }

    void AssetBrowser::Refresh()
    {
        BuildFileTree();
        RefreshCurrentDirectory();
        Engine::Core::Logger::Instance().Info("AssetBrowser refreshed");
    }

    void AssetBrowser::SetRootPath(const std::string& path)
    {
        m_rootPath = path;
        m_currentPath = path;
        Refresh();
    }

    void AssetBrowser::NavigateToPath(const std::string& path)
    {
        if (std::filesystem::exists(path) && std::filesystem::is_directory(path)) {
            m_currentPath = path;
            RefreshCurrentDirectory();
            Engine::Core::Logger::Instance().Info("Navigated to: " + path);
        }
    }

    void AssetBrowser::NavigateUp()
    {
        std::filesystem::path currentPath(m_currentPath);
        std::filesystem::path parentPath = currentPath.parent_path();
        
        if (!parentPath.empty() && parentPath != currentPath) {
            NavigateToPath(parentPath.string());
        }
    }

    void AssetBrowser::NavigateToParent()
    {
        NavigateUp();
    }

    void AssetBrowser::SelectAsset(const std::string& path)
    {
        for (size_t i = 0; i < m_filteredAssets.size(); ++i) {
            if (m_filteredAssets[i]->path == path) {
                SelectAsset(static_cast<int>(i));
                break;
            }
        }
    }

    void AssetBrowser::SelectAsset(int index)
    {
        if (index >= 0 && index < static_cast<int>(m_filteredAssets.size())) {
            if (!m_multiSelect) {
                ClearSelection();
            }
            
            m_selectedIndex = index;
            auto asset = m_filteredAssets[index];
            asset->isSelected = true;
            m_selectedAssets.push_back(asset);
            
            std::string typeStr = GetAssetIcon(asset->type); // Use icon as type identifier
            std::cout << "[AssetBrowser] ✓ Selected asset #" << index << ": '" << asset->name 
                      << "' (" << typeStr << ")" << std::endl;
            Engine::Core::Logger::Instance().Info("AssetBrowser: Selected asset '" + asset->name + "'");
            
            if (m_assetSelectedCallback) {
                m_assetSelectedCallback(asset->path);
            }
        } else {
            std::cout << "[AssetBrowser] ✗ Invalid asset index: " << index 
                      << " (max: " << (m_filteredAssets.size() - 1) << ")" << std::endl;
        }
    }

    void AssetBrowser::ClearSelection()
    {
        for (auto& asset : m_selectedAssets) {
            asset->isSelected = false;
        }
        m_selectedAssets.clear();
        m_selectedIndex = -1;
    }

    std::vector<std::shared_ptr<AssetBrowser::AssetInfo>> AssetBrowser::GetSelectedAssets() const
    {
        return m_selectedAssets;
    }

    std::shared_ptr<AssetBrowser::AssetInfo> AssetBrowser::GetSelectedAsset() const
    {
        if (m_selectedIndex >= 0 && m_selectedIndex < static_cast<int>(m_filteredAssets.size())) {
            return m_filteredAssets[m_selectedIndex];
        }
        return nullptr;
    }

    void AssetBrowser::ImportAsset(const std::string& filePath)
    {
        if (std::filesystem::exists(filePath)) {
            std::filesystem::path sourcePath(filePath);
            std::filesystem::path destPath = std::filesystem::path(m_currentPath) / sourcePath.filename();
            
            try {
                std::filesystem::copy_file(sourcePath, destPath);
                RefreshCurrentDirectory();
                
                if (m_assetImportedCallback) {
                    m_assetImportedCallback(destPath.string());
                }
                
                Engine::Core::Logger::Instance().Info("Asset imported: " + destPath.string());
            } catch (const std::exception& e) {
                Engine::Core::Logger::Instance().Error("Failed to import asset: " + std::string(e.what()));
            }
        }
    }

    void AssetBrowser::DeleteAsset(const std::string& path)
    {
        if (std::filesystem::exists(path)) {
            try {
                if (std::filesystem::is_directory(path)) {
                    std::filesystem::remove_all(path);
                } else {
                    std::filesystem::remove(path);
                }
                RefreshCurrentDirectory();
                Engine::Core::Logger::Instance().Info("Asset deleted: " + path);
            } catch (const std::exception& e) {
                Engine::Core::Logger::Instance().Error("Failed to delete asset: " + std::string(e.what()));
            }
        }
    }

    void AssetBrowser::RenameAsset(const std::string& oldPath, const std::string& newName)
    {
        if (std::filesystem::exists(oldPath)) {
            std::filesystem::path oldPathObj(oldPath);
            std::filesystem::path newPath = oldPathObj.parent_path() / newName;
            
            try {
                std::filesystem::rename(oldPath, newPath);
                RefreshCurrentDirectory();
                Engine::Core::Logger::Instance().Info("Asset renamed: " + oldPath + " -> " + newPath.string());
            } catch (const std::exception& e) {
                Engine::Core::Logger::Instance().Error("Failed to rename asset: " + std::string(e.what()));
            }
        }
    }

    void AssetBrowser::CopyAsset(const std::string& sourcePath, const std::string& destPath)
    {
        if (std::filesystem::exists(sourcePath)) {
            try {
                if (std::filesystem::is_directory(sourcePath)) {
                    std::filesystem::copy(sourcePath, destPath, std::filesystem::copy_options::recursive);
                } else {
                    std::filesystem::copy_file(sourcePath, destPath);
                }
                RefreshCurrentDirectory();
                Engine::Core::Logger::Instance().Info("Asset copied: " + sourcePath + " -> " + destPath);
            } catch (const std::exception& e) {
                Engine::Core::Logger::Instance().Error("Failed to copy asset: " + std::string(e.what()));
            }
        }
    }

    void AssetBrowser::MoveAsset(const std::string& sourcePath, const std::string& destPath)
    {
        if (std::filesystem::exists(sourcePath)) {
            try {
                std::filesystem::rename(sourcePath, destPath);
                RefreshCurrentDirectory();
                Engine::Core::Logger::Instance().Info("Asset moved: " + sourcePath + " -> " + destPath);
            } catch (const std::exception& e) {
                Engine::Core::Logger::Instance().Error("Failed to move asset: " + std::string(e.what()));
            }
        }
    }

    void AssetBrowser::SetFilter(AssetType type)
    {
        m_filterType = type;
        m_filterExtension.clear();
        UpdateFilteredAssets();
    }

    void AssetBrowser::SetFilter(const std::string& extension)
    {
        m_filterExtension = extension;
        m_filterType = AssetType::Unknown;
        UpdateFilteredAssets();
    }

    void AssetBrowser::ClearFilter()
    {
        m_filterType = AssetType::Unknown;
        m_filterExtension.clear();
        UpdateFilteredAssets();
    }

    void AssetBrowser::Search(const std::string& query)
    {
        m_searchQuery = query;
        UpdateFilteredAssets();
    }

    void AssetBrowser::ClearSearch()
    {
        m_searchQuery.clear();
        UpdateFilteredAssets();
    }

    void AssetBrowser::SetViewMode(bool listView)
    {
        m_listView = listView;
        m_itemHeight = listView ? 20 : m_thumbnailSize + m_thumbnailPadding * 2;
    }

    void AssetBrowser::SetThumbnailSize(int size)
    {
        m_thumbnailSize = std::max(MIN_THUMBNAIL_SIZE, std::min(MAX_THUMBNAIL_SIZE, size));
        if (!m_listView) {
            m_itemHeight = m_thumbnailSize + m_thumbnailPadding * 2;
        }
    }

    void AssetBrowser::SetShowHiddenFiles(bool show)
    {
        m_showHiddenFiles = show;
        RefreshCurrentDirectory();
    }

    void AssetBrowser::StartDrag(const std::string& assetPath)
    {
        m_dragging = true;
        m_dragAssetPath = assetPath;
    }

    void AssetBrowser::HandleDrop(int x, int y, const std::string& filePath)
    {
        (void)x; (void)y; // Suppress unused parameter warnings
        ImportAsset(filePath);
    }

    void AssetBrowser::SetAssetSelectedCallback(std::function<void(const std::string&)> callback)
    {
        m_assetSelectedCallback = callback;
    }

    void AssetBrowser::SetAssetDoubleClickedCallback(std::function<void(const std::string&)> callback)
    {
        m_assetDoubleClickedCallback = callback;
    }

    void AssetBrowser::SetAssetImportedCallback(std::function<void(const std::string&)> callback)
    {
        m_assetImportedCallback = callback;
    }

    void AssetBrowser::BuildFileTree()
    {
        std::cout << "DEBUG: BuildFileTree() START" << std::endl;
        Engine::Core::Logger::Instance().Debug("BuildFileTree() START");
        
        m_rootNode = std::make_shared<FolderNode>();
        m_rootNode->name = "Assets";
        m_rootNode->path = m_rootPath;
        m_rootNode->isExpanded = true;
        
        std::cout << "DEBUG: About to call BuildFileTreeRecursive()" << std::endl;
        BuildFileTreeRecursive(m_rootNode, m_rootPath);
        std::cout << "DEBUG: BuildFileTreeRecursive() completed" << std::endl;
        
        std::cout << "DEBUG: BuildFileTree() END" << std::endl;
        Engine::Core::Logger::Instance().Debug("BuildFileTree() END");
    }

    void AssetBrowser::BuildFileTreeRecursive(std::shared_ptr<FolderNode> node, const std::filesystem::path& path)
    {
        try {
            for (const auto& entry : std::filesystem::directory_iterator(path)) {
                if (entry.is_directory()) {
                    auto childNode = std::make_shared<FolderNode>();
                    childNode->name = entry.path().filename().string();
                    childNode->path = entry.path().string();
                    childNode->level = node->level + 1;
                    node->children.push_back(childNode);
                    
                    // Recursively build subdirectories
                    BuildFileTreeRecursive(childNode, entry.path());
                }
            }
        } catch (const std::exception& e) {
            Engine::Core::Logger::Instance().Error("Error building file tree: " + std::string(e.what()));
        }
    }

    void AssetBrowser::RefreshCurrentDirectory()
    {
        std::cout << "DEBUG: RefreshCurrentDirectory() START" << std::endl;
        Engine::Core::Logger::Instance().Debug("RefreshCurrentDirectory() START");
        
        m_currentAssets.clear();
        
        try {
            for (const auto& entry : std::filesystem::directory_iterator(m_currentPath)) {
                auto asset = std::make_shared<AssetInfo>();
                asset->name = entry.path().filename().string();
                asset->path = entry.path().string();
                asset->relativePath = std::filesystem::relative(entry.path(), m_rootPath).string();
                asset->isDirectory = entry.is_directory();
                asset->fileSize = entry.is_regular_file() ? std::filesystem::file_size(entry) : 0;
                
                // Get last modified time
                auto time = std::filesystem::last_write_time(entry);
                auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
                    time - std::filesystem::file_time_type::clock::now() + std::chrono::system_clock::now());
                auto time_t = std::chrono::system_clock::to_time_t(sctp);
                std::stringstream ss;
                struct tm timeinfo;
                localtime_s(&timeinfo, &time_t);
                ss << std::put_time(&timeinfo, "%Y-%m-%d %H:%M:%S");
                asset->lastModified = ss.str();
                
                // Determine asset type
                if (asset->isDirectory) {
                    asset->type = AssetType::Folder;
                } else {
                    std::string extension = entry.path().extension().string();
                    std::transform(extension.begin(), extension.end(), extension.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
                    asset->type = GetAssetTypeFromExtension(extension);
                }
                
                // Generate thumbnail
                GenerateThumbnail(asset);
                
                m_currentAssets.push_back(asset);
            }
        } catch (const std::exception& e) {
            Engine::Core::Logger::Instance().Error("Error refreshing directory: " + std::string(e.what()));
        }
        
        // Sort assets (folders first, then by name)
        std::sort(m_currentAssets.begin(), m_currentAssets.end(), 
            [](const std::shared_ptr<AssetInfo>& a, const std::shared_ptr<AssetInfo>& b) {
                if (a->isDirectory != b->isDirectory) {
                    return a->isDirectory > b->isDirectory;
                }
                return a->name < b->name;
            });
        
        UpdateFilteredAssets();
        
        std::cout << "DEBUG: RefreshCurrentDirectory() END - found " << m_currentAssets.size() << " assets" << std::endl;
        Engine::Core::Logger::Instance().Debug("RefreshCurrentDirectory() END - found " + std::to_string(m_currentAssets.size()) + " assets");
    }

    void AssetBrowser::UpdateFilteredAssets()
    {
        Engine::Core::Logger::Instance().Debug("=== UpdateFilteredAssets() START ===");
        m_filteredAssets.clear();
        
        Engine::Core::Logger::Instance().Debug("Checking " + std::to_string(m_currentAssets.size()) + " assets");
        Engine::Core::Logger::Instance().Debug("m_filterType: " + std::to_string(static_cast<int>(m_filterType)));
        Engine::Core::Logger::Instance().Debug("m_searchQuery: '" + m_searchQuery + "'");
        Engine::Core::Logger::Instance().Debug("m_filterExtension: '" + m_filterExtension + "'");
        Engine::Core::Logger::Instance().Debug("m_showHiddenFiles: " + std::to_string(m_showHiddenFiles));
        
        for (auto& asset : m_currentAssets) {
            bool isVisible = IsAssetVisible(asset);
            if (isVisible) {
                m_filteredAssets.push_back(asset);
                Engine::Core::Logger::Instance().Debug("Asset VISIBLE: " + asset->name);
            } else {
                Engine::Core::Logger::Instance().Debug("Asset FILTERED OUT: " + asset->name);
            }
        }
        
        // Update selection index
        if (m_selectedIndex >= static_cast<int>(m_filteredAssets.size())) {
            m_selectedIndex = -1;
        }
        
        Engine::Core::Logger::Instance().Debug("=== UpdateFilteredAssets() COMPLETED ===");
        Engine::Core::Logger::Instance().Debug("Final m_filteredAssets size: " + std::to_string(m_filteredAssets.size()));
    }

    void AssetBrowser::RenderFolderTree(HDC hdc)
    {
        int treeY = m_y + 65;
        int treeWidth = m_width / 3;
        
        // Draw folder tree background
        HBRUSH hBrush = CreateSolidBrush(RGB(40, 40, 40));
        RECT treeRect = { m_x + 5, treeY, m_x + treeWidth, m_y + m_height - 5 };
        FillRect(hdc, &treeRect, hBrush);
        DeleteObject(hBrush);
        
        // Draw folder tree
        if (m_rootNode) {
            RenderFolderNode(hdc, m_rootNode, treeY);
        }
    }

    void AssetBrowser::RenderFolderNode(HDC hdc, std::shared_ptr<FolderNode> node, int& y)
    {
        if (y > m_y + m_height - 30) return; // Don't render if outside visible area
        
        // Draw folder icon and name
        int indent = node->level * TREE_INDENT;
        RECT textRect = { m_x + 10 + indent, y, m_x + m_width / 3 - 10, y + 20 };
        
        // Highlight selected folder
        if (node->isSelected) {
            HBRUSH hBrush = CreateSolidBrush(RGB(60, 80, 100));
            FillRect(hdc, &textRect, hBrush);
            DeleteObject(hBrush);
        }
        
        // Draw folder icon
        std::string icon = node->isExpanded ? "📂" : "📁";
        std::wstring wideIcon(icon.begin(), icon.end());
        ::SetTextColor(hdc, RGB(255, 255, 255));
        SetBkMode(hdc, TRANSPARENT);
        DrawText(hdc, wideIcon.c_str(), -1, &textRect, DT_LEFT | DT_VCENTER);
        
        // Draw folder name
        textRect.left += 25;
        std::wstring wideName(node->name.begin(), node->name.end());
        DrawText(hdc, wideName.c_str(), -1, &textRect, DT_LEFT | DT_VCENTER);
        
        y += 20;
        
        // Render children if expanded
        if (node->isExpanded) {
            for (auto& child : node->children) {
                RenderFolderNode(hdc, child, y);
            }
        }
    }

    void AssetBrowser::RenderAssetList(HDC hdc)
    {
        int treeWidth = m_width / 3;
        int startX = m_x + treeWidth + 10;
        int startY = m_y + 70; // Moved down to avoid overlap with search bar
        int listWidth = m_width - treeWidth - 15;
        int listHeight = m_height - 75; // Adjusted for new startY
        
        // Draw asset list background
        HBRUSH hBrush = CreateSolidBrush(RGB(50, 50, 50));
        RECT listRect = { startX, startY, startX + listWidth, startY + listHeight };
        FillRect(hdc, &listRect, hBrush);
        DeleteObject(hBrush);
        
        // DEBUG: Log asset counts
        static int renderCount = 0;
        if (renderCount++ % 100 == 0) { // Log every 100 renders to avoid spam
            Engine::Core::Logger::Instance().Debug("RenderAssetList - m_currentAssets: " + std::to_string(m_currentAssets.size()) + 
                                                    ", m_filteredAssets: " + std::to_string(m_filteredAssets.size()));
        }
        
        // Render assets
        int currentY = startY + 5;
        
        // DEBUG: Always render some test content to see if rendering works
        if (m_filteredAssets.empty()) {
            std::wstring testText = L"DEBUG: No filtered assets found!";
            ::SetTextColor(hdc, RGB(255, 0, 0)); // Red text
            SetBkMode(hdc, TRANSPARENT);
            RECT testRect = { startX + 5, currentY, startX + listWidth - 10, currentY + 20 };
            DrawText(hdc, testText.c_str(), -1, &testRect, DT_LEFT | DT_VCENTER);
            currentY += 25;
            
            // Also show current assets count
            std::string debugMsg = "Current assets: " + std::to_string(m_currentAssets.size()) + 
                                 ", Filtered: " + std::to_string(m_filteredAssets.size());
            std::wstring wideDebug(debugMsg.begin(), debugMsg.end());
            ::SetTextColor(hdc, RGB(255, 255, 0)); // Yellow text
            RECT debugRect = { startX + 5, currentY, startX + listWidth - 10, currentY + 20 };
            DrawText(hdc, wideDebug.c_str(), -1, &debugRect, DT_LEFT | DT_VCENTER);
            currentY += 25;
        }
        
        for (size_t i = 0; i < m_filteredAssets.size(); ++i) {
            if (currentY > startY + listHeight) break; // Don't render if outside visible area
            
            auto asset = m_filteredAssets[i];
            int itemHeight = m_listView ? 28 : m_thumbnailSize + m_thumbnailPadding * 2; // Increased from 20 to 28
            
            RenderAssetItem(hdc, asset, startX + 5, currentY, listWidth - 10, itemHeight);
            currentY += itemHeight + 3; // Increased spacing from 2 to 3
        }
    }

    void AssetBrowser::RenderAssetItem(HDC hdc, std::shared_ptr<AssetInfo> asset, int x, int y, int width, int height)
    {
        // Highlight selected asset
        if (asset->isSelected) {
            HBRUSH hBrush = CreateSolidBrush(RGB(60, 80, 100));
            RECT highlightRect = { x, y, x + width, y + height };
            FillRect(hdc, &highlightRect, hBrush);
            DeleteObject(hBrush);
        }
        
        if (m_listView) {
            // List view - Fixed text rendering and spacing
            RECT textRect = { x + 30, y + 2, x + width - 80, y + height - 2 }; // Increased padding and space for size
            
            // Draw icon with better spacing
            std::string icon = GetAssetIcon(asset->type);
            std::wstring wideIcon(icon.begin(), icon.end());
            ::SetTextColor(hdc, RGB(255, 255, 255));
            SetBkMode(hdc, TRANSPARENT);
            RECT iconRect = { x + 8, y + 2, x + 25, y + height - 2 };
            DrawText(hdc, wideIcon.c_str(), -1, &iconRect, DT_LEFT | DT_VCENTER);
            
            // Draw name with proper clipping
            std::wstring wideName(asset->name.begin(), asset->name.end());
            DrawText(hdc, wideName.c_str(), -1, &textRect, DT_LEFT | DT_VCENTER | DT_WORDBREAK);
            
            // Draw size with more space and better positioning
            if (!asset->isDirectory) {
                std::string sizeStr = std::to_string(asset->fileSize / 1024) + " KB";
                std::wstring wideSize(sizeStr.begin(), sizeStr.end());
                RECT sizeRect = { x + width - 75, y + 2, x + width - 5, y + height - 2 };
                ::SetTextColor(hdc, RGB(200, 200, 200)); // Lighter color for size
                DrawText(hdc, wideSize.c_str(), -1, &sizeRect, DT_RIGHT | DT_VCENTER);
            }
        } else {
            // Grid view
            RenderThumbnail(hdc, asset, x, y, width, height);
        }
    }

    void AssetBrowser::RenderThumbnail(HDC hdc, std::shared_ptr<AssetInfo> asset, int x, int y, int width, int height)
    {
        int thumbX = x + (width - m_thumbnailSize) / 2;
        int thumbY = y + (height - m_thumbnailSize) / 2;
        
        if (asset->thumbnail) {
            // Draw thumbnail
            HDC memDC = CreateCompatibleDC(hdc);
            HGDIOBJ oldBitmap = SelectObject(memDC, asset->thumbnail);
            
            StretchBlt(hdc, thumbX, thumbY, m_thumbnailSize, m_thumbnailSize,
                      memDC, 0, 0, asset->thumbnailWidth, asset->thumbnailHeight, SRCCOPY);
            
            SelectObject(memDC, oldBitmap);
            DeleteDC(memDC);
        } else {
            // Draw placeholder
            HBRUSH hBrush = CreateSolidBrush(RGB(80, 80, 80));
            RECT thumbRect = { thumbX, thumbY, thumbX + m_thumbnailSize, thumbY + m_thumbnailSize };
            FillRect(hdc, &thumbRect, hBrush);
            DeleteObject(hBrush);
            
            // Draw icon
            std::string icon = GetAssetIcon(asset->type);
            std::wstring wideIcon(icon.begin(), icon.end());
            ::SetTextColor(hdc, RGB(255, 255, 255));
            SetBkMode(hdc, TRANSPARENT);
            RECT iconRect = { thumbX, thumbY, thumbX + m_thumbnailSize, thumbY + m_thumbnailSize };
            DrawText(hdc, wideIcon.c_str(), -1, &iconRect, DT_CENTER | DT_VCENTER);
        }
        
        // Draw name below thumbnail
        RECT nameRect = { x, y + m_thumbnailSize + 5, x + width, y + height };
        std::wstring wideName(asset->name.begin(), asset->name.end());
        ::SetTextColor(hdc, RGB(255, 255, 255));
        DrawText(hdc, wideName.c_str(), -1, &nameRect, DT_CENTER | DT_TOP);
    }

    void AssetBrowser::RenderToolbar(HDC hdc)
    {
        int toolbarY = m_y + 30;
        int toolbarHeight = 25;
        
        // Draw toolbar background
        HBRUSH hBrush = CreateSolidBrush(RGB(60, 60, 60));
        RECT toolbarRect = { m_x + 5, toolbarY, m_x + m_width - 5, toolbarY + toolbarHeight };
        FillRect(hdc, &toolbarRect, hBrush);
        DeleteObject(hBrush);
        
        // Draw toolbar buttons
        std::vector<std::string> buttons = { "Refresh", "Import", "View", "Filter" };
        int buttonWidth = (m_width - 20) / static_cast<int>(buttons.size());
        
        for (size_t i = 0; i < buttons.size(); ++i) {
            RECT buttonRect = { m_x + 10 + static_cast<int>(i) * buttonWidth, toolbarY + 2, 
                               m_x + 10 + static_cast<int>(i + 1) * buttonWidth, toolbarY + toolbarHeight - 2 };
            
            // Draw button
            HBRUSH buttonBrush = CreateSolidBrush(RGB(80, 80, 80));
            FillRect(hdc, &buttonRect, buttonBrush);
            DeleteObject(buttonBrush);
            
            // Draw button text
            std::wstring wideText(buttons[i].begin(), buttons[i].end());
            ::SetTextColor(hdc, RGB(255, 255, 255));
            SetBkMode(hdc, TRANSPARENT);
            DrawText(hdc, wideText.c_str(), -1, &buttonRect, DT_CENTER | DT_VCENTER);
        }
    }

    void AssetBrowser::RenderSearchBar(HDC hdc)
    {
        int searchY = m_y + 35;
        int searchHeight = 20;
        
        // Draw search bar background
        HBRUSH hBrush = CreateSolidBrush(RGB(60, 60, 60));
        RECT searchRect = { m_x + 5, searchY, m_x + m_width - 5, searchY + searchHeight };
        FillRect(hdc, &searchRect, hBrush);
        DeleteObject(hBrush);
        
        // Draw search text
        std::string searchText = "Search: " + m_searchQuery;
        std::wstring wideText(searchText.begin(), searchText.end());
        ::SetTextColor(hdc, RGB(200, 200, 200));
        SetBkMode(hdc, TRANSPARENT);
        RECT textRect = { m_x + 10, searchY, m_x + m_width - 10, searchY + searchHeight };
        DrawText(hdc, wideText.c_str(), -1, &textRect, DT_LEFT | DT_VCENTER);
    }

    void AssetBrowser::HandleFolderClick(int x, int y)
    {
        // TODO: Implement folder click handling
        (void)x; (void)y; // Suppress unused parameter warnings
    }

    void AssetBrowser::HandleAssetClick(int x, int y)
    {
        // x, y are already local coordinates relative to AssetBrowser
        std::cout << "[AssetBrowser] ========================================" << std::endl;
        std::cout << "[AssetBrowser] HandleAssetClick START" << std::endl;
        std::cout << "[AssetBrowser] Input coordinates: x=" << x << ", y=" << y << " (local to AssetBrowser)" << std::endl;
        std::cout << "[AssetBrowser] AssetBrowser bounds: [" << m_x << "," << (m_x + m_width) << "] x [" << m_y << "," << (m_y + m_height) << "]" << std::endl;
        std::cout << "[AssetBrowser] AssetBrowser size: " << m_width << "x" << m_height << std::endl;
        
        int treeWidth = m_width / 3;
        int startX = treeWidth + 10;
        int startY = 65;
        
        std::cout << "[AssetBrowser] Asset area: startX=" << startX << ", startY=" << startY 
                  << ", treeWidth=" << treeWidth << std::endl;
        
        int relativeX = x - startX;
        int relativeY = y - startY;
        
        std::cout << "[AssetBrowser] Relative coordinates: relativeX=" << relativeX << ", relativeY=" << relativeY << std::endl;
        std::cout << "[AssetBrowser] Asset list size: " << m_filteredAssets.size() << std::endl;
        
        // Check if click is in asset area
        if (relativeX < 0 || relativeY < 0) {
            std::cout << "[AssetBrowser] ✗ Click is BEFORE asset area start (relativeX=" << relativeX 
                      << ", relativeY=" << relativeY << ")" << std::endl;
            std::cout << "[AssetBrowser] This might be a click on the folder tree or search bar" << std::endl;
            return;
        }
        
        int assetIndex = GetAssetIndexFromPosition(relativeX, relativeY);
        std::cout << "[AssetBrowser] GetAssetIndexFromPosition(" << relativeX << "," << relativeY 
                  << ") returned: " << assetIndex << std::endl;
        std::cout << "[AssetBrowser] Item height: " << m_itemHeight << ", listView mode: " << m_listView << std::endl;
        
        if (assetIndex >= 0 && assetIndex < static_cast<int>(m_filteredAssets.size())) {
            std::cout << "[AssetBrowser] ✓ Valid asset index, selecting..." << std::endl;
            SelectAsset(assetIndex);
        } else {
            std::cout << "[AssetBrowser] ✗ Click didn't hit any asset (index=" << assetIndex 
                      << ", assets=" << m_filteredAssets.size() << ")" << std::endl;
            if (assetIndex < 0) {
                std::cout << "[AssetBrowser]   Reason: Negative index (click might be outside asset list)" << std::endl;
            } else if (assetIndex >= static_cast<int>(m_filteredAssets.size())) {
                std::cout << "[AssetBrowser]   Reason: Index out of bounds (clicked below last asset)" << std::endl;
            }
        }
        std::cout << "[AssetBrowser] HandleAssetClick END" << std::endl;
        std::cout << "[AssetBrowser] ========================================" << std::endl;
    }

    void AssetBrowser::HandleAssetDoubleClick(int x, int y)
    {
        int treeWidth = m_width / 3;
        int startX = m_x + treeWidth + 10;
        int startY = m_y + 65;
        
        int relativeX = x - startX;
        int relativeY = y - startY;
        
        int assetIndex = GetAssetIndexFromPosition(relativeX, relativeY);
        if (assetIndex >= 0 && assetIndex < static_cast<int>(m_filteredAssets.size())) {
            auto asset = m_filteredAssets[assetIndex];
            if (asset->isDirectory) {
                NavigateToPath(asset->path);
            } else if (m_assetDoubleClickedCallback) {
                m_assetDoubleClickedCallback(asset->path);
            }
        }
    }

    void AssetBrowser::HandleRightClick(int x, int y)
    {
        // TODO: Implement context menu
        (void)x; (void)y; // Suppress unused parameter warnings
    }

    AssetBrowser::AssetType AssetBrowser::GetAssetTypeFromExtension(const std::string& extension)
    {
        if (extension == ".fbx" || extension == ".obj" || extension == ".gltf" || extension == ".blend") {
            return AssetType::Model3D;
        } else if (extension == ".png" || extension == ".jpg" || extension == ".jpeg" || 
                   extension == ".tga" || extension == ".exr" || extension == ".hdr") {
            return AssetType::Image;
        } else if (extension == ".wav" || extension == ".mp3" || extension == ".ogg") {
            return AssetType::Audio;
        } else if (extension == ".mp4" || extension == ".mov" || extension == ".avi") {
            return AssetType::Video;
        } else if (extension == ".hlsl" || extension == ".glsl") {
            return AssetType::Shader;
        } else if (extension == ".ams") {
            return AssetType::Scene;
        } else if (extension == ".template") {
            return AssetType::Template;
        }
        return AssetType::Unknown;
    }

    std::string AssetBrowser::GetAssetIcon(AssetType type)
    {
        switch (type) {
            case AssetType::Folder: return "📁";
            case AssetType::Model3D: return "🎲";
            case AssetType::Image: return "🖼️";
            case AssetType::Audio: return "🎵";
            case AssetType::Video: return "🎬";
            case AssetType::Material: return "🎨";
            case AssetType::Shader: return "⚡";
            case AssetType::Scene: return "🎭";
            case AssetType::Template: return "📋";
            default: return "📄";
        }
    }

    HBITMAP AssetBrowser::LoadThumbnail(const std::string& filePath, AssetType type)
    {
        // TODO: Implement thumbnail loading for different asset types
        (void)filePath; (void)type; // Suppress unused parameter warnings
        return nullptr;
    }

    void AssetBrowser::GenerateThumbnail(std::shared_ptr<AssetInfo> asset)
    {
        if (asset->type == AssetType::Image) {
            // TODO: Load image thumbnail
            asset->thumbnail = LoadThumbnail(asset->path, asset->type);
            if (asset->thumbnail) {
                BITMAP bmp;
                GetObject(asset->thumbnail, sizeof(BITMAP), &bmp);
                asset->thumbnailWidth = bmp.bmWidth;
                asset->thumbnailHeight = bmp.bmHeight;
            }
        }
    }

    bool AssetBrowser::IsAssetVisible(std::shared_ptr<AssetInfo> asset)
    {
        Engine::Core::Logger::Instance().Debug("IsAssetVisible() checking: " + asset->name);
        
        // Check if hidden files should be shown
        if (!m_showHiddenFiles && asset->name[0] == '.') {
            Engine::Core::Logger::Instance().Debug("  -> FILTERED: Hidden file");
            return false;
        }
        
        // Check filter
        if (!MatchesFilter(asset)) {
            Engine::Core::Logger::Instance().Debug("  -> FILTERED: MatchesFilter() returned false");
            return false;
        }
        
        // Check search
        if (!MatchesSearch(asset)) {
            Engine::Core::Logger::Instance().Debug("  -> FILTERED: MatchesSearch() returned false");
            return false;
        }
        
        Engine::Core::Logger::Instance().Debug("  -> VISIBLE: All checks passed");
        return true;
    }

    bool AssetBrowser::MatchesSearch(std::shared_ptr<AssetInfo> asset)
    {
        Engine::Core::Logger::Instance().Debug("  MatchesSearch() for: " + asset->name + " (query: '" + m_searchQuery + "')");
        
        if (m_searchQuery.empty()) {
            Engine::Core::Logger::Instance().Debug("    No search query -> MATCH");
            return true;
        }
        
        std::string name = asset->name;
        std::string query = m_searchQuery;
        std::transform(name.begin(), name.end(), name.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        std::transform(query.begin(), query.end(), query.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        
        bool matches = (name.find(query) != std::string::npos);
        Engine::Core::Logger::Instance().Debug("    Search: '" + query + "' in '" + name + "' -> " + (matches ? "MATCH" : "NO MATCH"));
        return matches;
    }

    bool AssetBrowser::MatchesFilter(std::shared_ptr<AssetInfo> asset)
    {
        Engine::Core::Logger::Instance().Debug("  MatchesFilter() for: " + asset->name + " (type: " + std::to_string(static_cast<int>(asset->type)) + ")");
        
        if (m_filterType != AssetType::Unknown) {
            bool matches = (asset->type == m_filterType);
            Engine::Core::Logger::Instance().Debug("    Filter by type: " + std::to_string(static_cast<int>(m_filterType)) + " -> " + (matches ? "MATCH" : "NO MATCH"));
            return matches;
        }
        
        if (!m_filterExtension.empty()) {
            std::string extension = std::filesystem::path(asset->path).extension().string();
            std::transform(extension.begin(), extension.end(), extension.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
            bool matches = (extension == m_filterExtension);
            Engine::Core::Logger::Instance().Debug("    Filter by extension: '" + m_filterExtension + "' vs '" + extension + "' -> " + (matches ? "MATCH" : "NO MATCH"));
            return matches;
        }
        
        Engine::Core::Logger::Instance().Debug("    No filter applied -> MATCH");
        return true;
    }

    void AssetBrowser::UpdateScrollbar()
    {
        // TODO: Implement scrollbar
    }

    void AssetBrowser::ScrollToAsset(int index)
    {
        (void)index; // Suppress unused parameter warning
        // TODO: Implement scrolling
    }

    int AssetBrowser::GetAssetIndexFromPosition(int x, int y)
    {
        if (m_listView) {
            return y / (m_itemHeight + 2);
        } else {
            int itemsPerRow = (m_width - m_width / 3 - 15) / (m_thumbnailSize + m_thumbnailPadding * 2);
            int row = y / (m_thumbnailSize + m_thumbnailPadding * 2);
            int col = x / (m_thumbnailSize + m_thumbnailPadding * 2);
            return row * itemsPerRow + col;
        }
    }

    std::shared_ptr<AssetBrowser::AssetInfo> AssetBrowser::GetAssetFromPosition(int x, int y)
    {
        int index = GetAssetIndexFromPosition(x, y);
        if (index >= 0 && index < static_cast<int>(m_filteredAssets.size())) {
            return m_filteredAssets[index];
        }
        return nullptr;
    }

    bool AssetBrowser::CreateDirectory(const std::string& path)
    {
        try {
            return std::filesystem::create_directories(path);
        } catch (const std::exception& e) {
            Engine::Core::Logger::Instance().Error("Failed to create directory: " + std::string(e.what()));
            return false;
        }
    }

    bool AssetBrowser::DeleteFile(const std::string& path)
    {
        try {
            return std::filesystem::remove(path);
        } catch (const std::exception& e) {
            Engine::Core::Logger::Instance().Error("Failed to delete file: " + std::string(e.what()));
            return false;
        }
    }

    bool AssetBrowser::RenameFile(const std::string& oldPath, const std::string& newPath)
    {
        try {
            std::filesystem::rename(oldPath, newPath);
            return true;
        } catch (const std::exception& e) {
            Engine::Core::Logger::Instance().Error("Failed to rename file: " + std::string(e.what()));
            return false;
        }
    }

    bool AssetBrowser::CopyFile(const std::string& sourcePath, const std::string& destPath)
    {
        try {
            std::filesystem::copy_file(sourcePath, destPath);
            return true;
        } catch (const std::exception& e) {
            Engine::Core::Logger::Instance().Error("Failed to copy file: " + std::string(e.what()));
            return false;
        }
    }

    bool AssetBrowser::MoveFile(const std::string& sourcePath, const std::string& destPath)
    {
        try {
            std::filesystem::rename(sourcePath, destPath);
            return true;
        } catch (const std::exception& e) {
            Engine::Core::Logger::Instance().Error("Failed to move file: " + std::string(e.what()));
            return false;
        }
    }

    void AssetBrowser::ShowContextMenu(int x, int y)
    {
        // TODO: Implement context menu
        (void)x; (void)y; // Suppress unused parameter warnings
    }

    void AssetBrowser::HandleContextMenuCommand(int commandId)
    {
        (void)commandId; // Suppress unused parameter warning
        // TODO: Implement context menu commands
    }

    void AssetBrowser::AddSampleAssets()
    {
        // Write to file for debugging
        std::ofstream debugFile("assetbrowser_debug.txt", std::ios::app);
        debugFile << "AddSampleAssets() called at " << std::time(nullptr) << std::endl;
        debugFile.close();
        
        std::cout << "DEBUG: AddSampleAssets() called!" << std::endl;
        Engine::Core::Logger::Instance().Debug("=== AddSampleAssets() START ===");
        Engine::Core::Logger::Instance().Debug("Current m_currentAssets size before: " + std::to_string(m_currentAssets.size()));
        
        // Add sample 3D models
        auto cubeModel = std::make_shared<AssetInfo>();
        cubeModel->name = "Cube.fbx";
        cubeModel->path = m_currentPath + "\\Cube.fbx";
        cubeModel->type = AssetType::Model3D;
        cubeModel->fileSize = 245760; // 240KB
        cubeModel->isDirectory = false;
        cubeModel->lastModified = std::to_string(std::time(nullptr));
        m_currentAssets.push_back(cubeModel);

        auto sphereModel = std::make_shared<AssetInfo>();
        sphereModel->name = "Sphere.obj";
        sphereModel->path = m_currentPath + "\\Sphere.obj";
        sphereModel->type = AssetType::Model3D;
        sphereModel->fileSize = 128000; // 125KB
        sphereModel->isDirectory = false;
        sphereModel->lastModified = std::to_string(std::time(nullptr));
        m_currentAssets.push_back(sphereModel);

        // Add sample textures
        auto woodTexture = std::make_shared<AssetInfo>();
        woodTexture->name = "Wood_Texture.jpg";
        woodTexture->path = m_currentPath + "\\Wood_Texture.jpg";
        woodTexture->type = AssetType::Image;
        woodTexture->fileSize = 1024000; // 1MB
        woodTexture->isDirectory = false;
        woodTexture->lastModified = std::to_string(std::time(nullptr));
        m_currentAssets.push_back(woodTexture);

        auto metalTexture = std::make_shared<AssetInfo>();
        metalTexture->name = "Metal_Texture.png";
        metalTexture->path = m_currentPath + "\\Metal_Texture.png";
        metalTexture->type = AssetType::Image;
        metalTexture->fileSize = 512000; // 500KB
        metalTexture->isDirectory = false;
        metalTexture->lastModified = std::to_string(std::time(nullptr));
        m_currentAssets.push_back(metalTexture);

        // Add sample materials
        auto plasticMaterial = std::make_shared<AssetInfo>();
        plasticMaterial->name = "Plastic_Material.mat";
        plasticMaterial->path = m_currentPath + "\\Plastic_Material.mat";
        plasticMaterial->type = AssetType::Material;
        plasticMaterial->fileSize = 2048; // 2KB
        plasticMaterial->isDirectory = false;
        plasticMaterial->lastModified = std::to_string(std::time(nullptr));
        m_currentAssets.push_back(plasticMaterial);

        // Add sample animations
        auto walkAnimation = std::make_shared<AssetInfo>();
        walkAnimation->name = "Walk_Animation.fbx";
        walkAnimation->path = m_currentPath + "\\Walk_Animation.fbx";
        walkAnimation->type = AssetType::Model3D; // Use Model3D for FBX files
        walkAnimation->fileSize = 512000; // 500KB
        walkAnimation->isDirectory = false;
        walkAnimation->lastModified = std::to_string(std::time(nullptr));
        m_currentAssets.push_back(walkAnimation);

        // Add sample audio
        auto backgroundMusic = std::make_shared<AssetInfo>();
        backgroundMusic->name = "Background_Music.wav";
        backgroundMusic->path = m_currentPath + "\\Background_Music.wav";
        backgroundMusic->type = AssetType::Audio;
        backgroundMusic->fileSize = 5120000; // 5MB
        backgroundMusic->isDirectory = false;
        backgroundMusic->lastModified = std::to_string(std::time(nullptr));
        m_currentAssets.push_back(backgroundMusic);

        // Sort assets (folders first, then by name)
        std::sort(m_currentAssets.begin(), m_currentAssets.end(), 
            [](const std::shared_ptr<AssetInfo>& a, const std::shared_ptr<AssetInfo>& b) {
                if (a->isDirectory != b->isDirectory) {
                    return a->isDirectory > b->isDirectory;
                }
                return a->name < b->name;
            });

        // Update filtered assets
        Engine::Core::Logger::Instance().Debug("About to call UpdateFilteredAssets()");
        UpdateFilteredAssets();
        
        Engine::Core::Logger::Instance().Debug("=== AddSampleAssets() COMPLETED ===");
        Engine::Core::Logger::Instance().Debug("Final m_currentAssets size: " + std::to_string(m_currentAssets.size()));
        Engine::Core::Logger::Instance().Debug("Final m_filteredAssets size: " + std::to_string(m_filteredAssets.size()));
        
        // Log each asset that was added
        for (size_t i = 0; i < m_currentAssets.size(); ++i) {
            Engine::Core::Logger::Instance().Debug("Asset " + std::to_string(i) + ": " + m_currentAssets[i]->name + " (type: " + std::to_string(static_cast<int>(m_currentAssets[i]->type)) + ")");
        }
    }

} // namespace UI
} // namespace Engine
