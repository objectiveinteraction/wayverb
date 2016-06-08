#pragma once

#include "FullModel.hpp"
#include "HelpWindow.hpp"

#include "../JuceLibraryCode/JuceHeader.h"

class MicrophoneListBox : public ListBox,
                          public ListBoxModel,
                          public model::BroadcastListener {
public:
    class Listener {
    public:
        Listener() = default;
        Listener(const Listener& rhs) = default;
        Listener& operator=(const Listener& rhs) = default;
        Listener(Listener&& rhs) noexcept = default;
        Listener& operator=(Listener&& rhs) noexcept = default;
        virtual ~Listener() noexcept = default;

        virtual void selectedRowsChanged(MicrophoneListBox* lb, int last) = 0;
    };

    MicrophoneListBox(
            model::ValueWrapper<std::vector<model::Microphone>>& microphones);

    int getNumRows() override;

    void paintListBoxItem(
            int row, Graphics& g, int w, int h, bool selected) override;

    Component* refreshComponentForRow(int row,
                                      bool selected,
                                      Component* existing) override;

    void receive_broadcast(model::Broadcaster* b) override;

    void selectedRowsChanged(int last) override;

    void addListener(Listener* l);
    void removeListener(Listener* l);

private:
    model::ValueWrapper<std::vector<model::Microphone>>& microphones;
    model::BroadcastConnector microphones_connector{&microphones, this};

    ListenerList<Listener> listener_list;
};

//----------------------------------------------------------------------------//

class MicrophoneEditableListBox : public Component,
                                  public MicrophoneListBox::Listener,
                                  public TextButton::Listener,
                                  public model::BroadcastListener {
public:
    class Listener {
    public:
        Listener() = default;
        Listener(const Listener& rhs) = default;
        Listener& operator=(const Listener& rhs) = default;
        Listener(Listener&& rhs) noexcept = default;
        Listener& operator=(Listener&& rhs) noexcept = default;
        virtual ~Listener() noexcept = default;

        virtual void selectedRowsChanged(MicrophoneEditableListBox* lb,
                                         int last) = 0;
    };

    MicrophoneEditableListBox(
            model::ValueWrapper<std::vector<model::Microphone>>& microphones);

    void buttonClicked(Button* b) override;

    void resized() override;

    void receive_broadcast(model::Broadcaster* b) override;

    void selectRow(int row);
    void selectedRowsChanged(MicrophoneListBox* lb, int last) override;

    void addListener(Listener* l);
    void removeListener(Listener* l);

private:
    void update_sub_button_enablement();

    model::ValueWrapper<std::vector<model::Microphone>>& microphones;
    model::BroadcastConnector microphones_connector{&microphones, this};

    MicrophoneListBox microphone_list_box;
    model::Connector<MicrophoneListBox> microphone_list_box_connector{
            &microphone_list_box, this};

    TextButton add_button{"+"};
    model::Connector<TextButton> add_button_connector{&add_button, this};

    TextButton sub_button{"-"};
    model::Connector<TextButton> sub_button_connector{&sub_button, this};

    ListenerList<Listener> listener_list;
};

//----------------------------------------------------------------------------//

class SingleMicrophoneComponent : public Component {
public:
    SingleMicrophoneComponent(
            model::ValueWrapper<model::Microphone>& microphone);

    void resized() override;

    int getTotalContentHeight() const;

private:
    PropertyPanel property_panel;
};

//----------------------------------------------------------------------------//

class MicrophoneEditorPanel : public Component,
                              public MicrophoneEditableListBox::Listener,
                              public SettableHelpPanelClient {
public:
    MicrophoneEditorPanel(
            model::ValueWrapper<std::vector<model::Microphone>>& microphones);

    void resized() override;

    void selectedRowsChanged(MicrophoneEditableListBox* lb, int last) override;

private:
    model::ValueWrapper<std::vector<model::Microphone>>& microphones;

    MicrophoneEditableListBox microphone_list_box;
    model::Connector<MicrophoneEditableListBox> microphone_list_box_connector{
            &microphone_list_box, this};

    std::unique_ptr<SingleMicrophoneComponent> single_microphone;
};