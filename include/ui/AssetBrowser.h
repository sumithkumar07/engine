#pragma once

#include <memory>
#include <vector>
#include <string>
#include <map>
#include <functional>
#include <filesystem>
#include <windows.h>
#include "UI.h"

namespace Engine {
namespace UI {

    /**
     * @brief Asset Browser for AI Movie Studio
     * 
     * Real implementation with actual functionality:
     * - File tree navigation with folder structure
     * - Asset previews (thumbnails for 3D models, images, etc.)
     * - Drag and drop support for importing assets
     * - File type filtering and search
     * - Asset management and organization
     * - Real-time file system monitoring
     */
    class AssetBrowser : public UIComponent {
    public:
        Type GetType() const override { return Type::Panel; }
    public:
        enum class AssetType {
            Unknown,
            Model3D,        // FBX, OBJ, GLTF, BLEND
            Image,          // PNG, JPG, TGA, EXR
            Audio,          // WAV, MP3, OGG
            Video,          // MP4, MOV, AVI
            Material,       // Custom material files
            Shader,         // HLSL, GLSL shader files
            Scene,          // Scene files
            Template,       // Template files
            Folder
        };

        struct AssetInfo {
            std::string name;
            std::string path;
            std::string relativePath;
            AssetType type;
            size_t fileSize;
            std::string lastModified;
            bool isDirectory;
            bool isSelected;
            bool isVisible;
            
            // Preview data
            HBITMAP thumbnail;
            int thumbnailWidth;
            int thumbnailHeight;
            
            AssetInfo() : type(AssetType::Unknown), fileSize(0), isDirectory(false), 
                         isSelected(false), isVisible(true), thumbnail(nullptr), 
                         thumbnailWidth(0), thumbnailHeight(0) {}
        };

        struct FolderNode {
            std::string name;
            std::string path;
            std::vector<std::shared_ptr<FolderNode>> children;
            std::vector<std::shared_ptr<AssetInfo>> assets;
            bool isExpanded;
            bool isSelected;
            int level; // For indentation
            
            FolderNode() : isExpanded(false), isSelected(false), level(0) {}
        };

        AssetBrowser();
        ~AssetBrowser();

        /**
         * @brief Initialize the asset browser
         * @param rootPath Root directory to browse
         * @return true if initialization successful
         */
        bool Initialize(const std::string& rootPath = "");

        /**
         * @brief Shutdown the asset browser
         */
        void Shutdown();

        /**
         * @brief Update the asset browser
         * @param deltaTime Time elapsed since last frame
         */
        void Update(float deltaTime) override;

        /**
         * @brief Render the asset browser
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

        // File system operations
        void Refresh();
        void SetRootPath(const std::string& path);
        std::string GetRootPath() const { return m_rootPath; }
        void NavigateToPath(const std::string& path);
        void NavigateUp();
        void NavigateToParent();

        // Asset operations
        void SelectAsset(const std::string& path);
        void SelectAsset(int index);
        void ClearSelection();
        std::vector<std::shared_ptr<AssetInfo>> GetSelectedAssets() const;
        std::shared_ptr<AssetInfo> GetSelectedAsset() const;
        
        // Asset management
        void ImportAsset(const std::string& filePath);
        void DeleteAsset(const std::string& path);
        void RenameAsset(const std::string& oldPath, const std::string& newName);
        void CopyAsset(const std::string& sourcePath, const std::string& destPath);
        void MoveAsset(const std::string& sourcePath, const std::string& destPath);

        // Filtering and search
        void SetFilter(AssetType type);
        void SetFilter(const std::string& extension);
        void ClearFilter();
        void Search(const std::string& query);
        void ClearSearch();

        // View options
        void SetViewMode(bool listView); // true = list, false = grid
        bool IsListView() const { return m_listView; }
        void SetThumbnailSize(int size);
        int GetThumbnailSize() const { return m_thumbnailSize; }
        void SetShowHiddenFiles(bool show);
        bool IsShowingHiddenFiles() const { return m_showHiddenFiles; }

        // Drag and drop
        void StartDrag(const std::string& assetPath);
        void HandleDrop(int x, int y, const std::string& filePath);
        bool IsDragging() const { return m_dragging; }

        // Callbacks
        void SetAssetSelectedCallback(std::function<void(const std::string&)> callback);
        void SetAssetDoubleClickedCallback(std::function<void(const std::string&)> callback);
        void SetAssetImportedCallback(std::function<void(const std::string&)> callback);

        // Getters
        const std::vector<std::shared_ptr<AssetInfo>>& GetCurrentAssets() const { return m_currentAssets; }
        const std::shared_ptr<FolderNode>& GetRootNode() const { return m_rootNode; }
        std::string GetCurrentPath() const { return m_currentPath; }

    private:
        // File system
        std::string m_rootPath;
        std::string m_currentPath;
        std::shared_ptr<FolderNode> m_rootNode;
        std::vector<std::shared_ptr<AssetInfo>> m_currentAssets;
        std::vector<std::shared_ptr<AssetInfo>> m_filteredAssets;
        
        // UI state
        bool m_listView;
        int m_thumbnailSize;
        bool m_showHiddenFiles;
        int m_scrollOffset;
        int m_itemHeight;
        int m_thumbnailPadding;
        
        // Selection
        std::vector<std::shared_ptr<AssetInfo>> m_selectedAssets;
        int m_selectedIndex;
        bool m_multiSelect;
        
        // Search and filter
        std::string m_searchQuery;
        AssetType m_filterType;
        std::string m_filterExtension;
        
        // Drag and drop
        bool m_dragging;
        std::string m_dragAssetPath;
        int m_dragStartX;
        int m_dragStartY;
        
        // Callbacks
        std::function<void(const std::string&)> m_assetSelectedCallback;
        std::function<void(const std::string&)> m_assetDoubleClickedCallback;
        std::function<void(const std::string&)> m_assetImportedCallback;
        
        // Helper methods
        void BuildFileTree();
        void BuildFileTreeRecursive(std::shared_ptr<FolderNode> node, const std::filesystem::path& path);
        void RefreshCurrentDirectory();
        void UpdateFilteredAssets();
        
        void RenderFolderTree(HDC hdc);
        void RenderFolderNode(HDC hdc, std::shared_ptr<FolderNode> node, int& y);
        void RenderAssetList(HDC hdc);
        void RenderAssetItem(HDC hdc, std::shared_ptr<AssetInfo> asset, int x, int y, int width, int height);
        void RenderThumbnail(HDC hdc, std::shared_ptr<AssetInfo> asset, int x, int y, int width, int height);
        void RenderToolbar(HDC hdc);
        void RenderSearchBar(HDC hdc);
        
        void HandleFolderClick(int x, int y);
        void HandleAssetClick(int x, int y);
        void HandleAssetDoubleClick(int x, int y);
        void HandleRightClick(int x, int y);
        
        AssetType GetAssetTypeFromExtension(const std::string& extension);
        std::string GetAssetIcon(AssetType type);
        HBITMAP LoadThumbnail(const std::string& filePath, AssetType type);
        void GenerateThumbnail(std::shared_ptr<AssetInfo> asset);
        
        bool IsAssetVisible(std::shared_ptr<AssetInfo> asset);
        bool MatchesSearch(std::shared_ptr<AssetInfo> asset);
        bool MatchesFilter(std::shared_ptr<AssetInfo> asset);
        
        void UpdateScrollbar();
        void ScrollToAsset(int index);
        int GetAssetIndexFromPosition(int x, int y);
        std::shared_ptr<AssetInfo> GetAssetFromPosition(int x, int y);
        
        // File operations
        bool CreateDirectory(const std::string& path);
        bool DeleteFile(const std::string& path);
        bool RenameFile(const std::string& oldPath, const std::string& newPath);
        bool CopyFile(const std::string& sourcePath, const std::string& destPath);
        bool MoveFile(const std::string& sourcePath, const std::string& destPath);
        
        // Context menu
        void ShowContextMenu(int x, int y);
        void HandleContextMenuCommand(int commandId);
        
        // Sample assets for demonstration
        void AddSampleAssets();
        
        // Constants
        static constexpr int DEFAULT_THUMBNAIL_SIZE = 64;
        static constexpr int MIN_THUMBNAIL_SIZE = 32;
        static constexpr int MAX_THUMBNAIL_SIZE = 128;
        static constexpr int ITEM_PADDING = 4;
        static constexpr int TREE_INDENT = 20;
    };

} // namespace UI
} // namespace Engine
