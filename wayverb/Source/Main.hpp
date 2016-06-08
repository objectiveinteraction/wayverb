#pragma once

#include "FileDropComponent.hpp"
#include "FullModel.hpp"
#include "ImpulseViewer.hpp"
#include "MainComponent.hpp"
#include "StoredSettings.hpp"
#include "VisualiserLookAndFeel.hpp"

class VisualiserApplication final : public JUCEApplication {
public:
    const String getApplicationName() override;
    const String getApplicationVersion() override;
    bool moreThanOneInstanceAllowed() override;

    void initialise(const String& commandLine) override;
    void shutdown() override;

    void systemRequestedQuit() override;
    void anotherInstanceStarted(const String& commandLine) override;

    static VisualiserApplication& get_app();
    static ApplicationCommandManager& get_command_manager();
    static std::string get_valid_file_formats();
    static PropertiesFile::Options get_property_file_options_for(
            const std::string& name);
    static constexpr auto recent_projects_base_id = 100;

    void create_file_menu(PopupMenu& menu);
    void create_view_menu(PopupMenu& menu);

    void handle_main_menu_command(int menu_item_id);

    void getAllCommands(Array<CommandID>& commands) override;
    void getCommandInfo(CommandID command_id,
                        ApplicationCommandInfo& result) override;
    bool perform(const InvocationInfo& info) override;

    void open_project(const File& file);
    void open_project_from_dialog();

    void show_hide_load_window();

    void attempt_close_all();

    class MainMenuBarModel : public MenuBarModel {
    public:
        MainMenuBarModel();

        StringArray getMenuBarNames() override;
        PopupMenu getMenuForIndex(int top_level_menu_index,
                                  const String& menu_name) override;
        void menuItemSelected(int menu_item_id,
                              int top_level_menu_index) override;
    };

    class MainWindow final : public DocumentWindow,
                             public ApplicationCommandTarget,
                             public model::BroadcastListener {
    public:
        //  load with a custom config too
        MainWindow(String name,
                   SceneData&& scene_data,
                   model::FullModel&& model,
                   File&& this_file);

        //  if the file is a .way, load a project, else just load like a 3d
        //  model
        MainWindow(String name, const File& f);
        virtual ~MainWindow() noexcept;

        void closeButtonPressed() override;

        bool needs_save() const;
        bool save_project();
        bool save_as_project();

        void show_help();

        void getAllCommands(Array<CommandID>& commands) override;
        void getCommandInfo(CommandID command_id,
                            ApplicationCommandInfo& result) override;
        bool perform(const InvocationInfo& info) override;
        ApplicationCommandTarget* getNextCommandTarget() override;

        void receive_broadcast(model::Broadcaster* b) override;

    private:
        MainWindow(String name,
                   std::tuple<SceneData, model::FullModel, File>&& p);

        void save_to(const File& f);

        static File get_model_path(const File& way);
        static File get_config_path(const File& way);
        static std::tuple<SceneData, model::FullModel, File>
        scene_and_model_from_file(const File& f);

        SceneData scene_data;
        model::FullModel model;
        model::ValueWrapper<model::FullModel> wrapper{nullptr, model};

        File this_file;
        MainContentComponent content_component;

        model::BroadcastConnector persistent_connector{&wrapper.persistent,
                                                       this};
        model::BroadcastConnector is_rendering_connector{
                &wrapper.render_state.is_rendering, this};
        model::BroadcastConnector visualising_connector{
                &wrapper.render_state.visualise, this};

        Component::SafePointer<DocumentWindow> help_window{nullptr};
    };

    class ImpulseViewerWindow : public DocumentWindow,
                                public ApplicationCommandTarget {
    public:
        ImpulseViewerWindow(String name, const File& file);
        virtual ~ImpulseViewerWindow() noexcept = default;

        void closeButtonPressed() override;

        void getAllCommands(Array<CommandID>& commands) override;
        void getCommandInfo(CommandID command_id,
                            ApplicationCommandInfo& result) override;
        bool perform(const InvocationInfo& info) override;
        ApplicationCommandTarget* getNextCommandTarget() override;

    private:
        AudioFormatManager audio_format_manager;
        File this_file;
        ImpulseViewer content_component;
    };

    std::unique_ptr<StoredSettings> stored_settings;

private:
    VisualiserLookAndFeel look_and_feel;
    SharedResourcePointer<TooltipWindow> tooltip_window;

    std::unique_ptr<ApplicationCommandManager> command_manager;
    std::unique_ptr<MainMenuBarModel> main_menu_bar_model;

    std::unique_ptr<DocumentWindow> load_window;
    std::unordered_set<std::unique_ptr<MainWindow>> main_windows;
    std::unordered_set<std::unique_ptr<ImpulseViewerWindow>> impulse_windows;

    class AsyncQuitRetrier : private Timer {
    public:
        AsyncQuitRetrier();
        virtual ~AsyncQuitRetrier() noexcept = default;

        AsyncQuitRetrier(const AsyncQuitRetrier&) = delete;
        AsyncQuitRetrier& operator=(const AsyncQuitRetrier&) = delete;
        AsyncQuitRetrier(AsyncQuitRetrier&&) = delete;
        AsyncQuitRetrier& operator=(AsyncQuitRetrier&&) = delete;

        void timerCallback() override;
    };
    std::unique_ptr<AsyncQuitRetrier> async_quit_retrier;
};