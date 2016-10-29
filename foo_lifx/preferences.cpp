#include "stdafx.h"
#include "preferences.h"

cfg_bool cfg_lifx_enabled(guid_cfg_lifx_enabled, true);
cfg_uint cfg_lifx_cycle_speed(guid_cfg_lifx_cycle_speed, 30);
cfg_uint cfg_lifx_brightness(guid_cfg_lifx_brightness, 100);

class CLifxPreferences : public CDialogImpl<CLifxPreferences>, public preferences_page_instance {
private:
	const preferences_page_callback::ptr m_callback;
	CCheckBox m_lifx_enabled; 

	BOOL OnInitDialog(CWindow, LPARAM) {
		m_lifx_enabled = GetDlgItem(IDC_LIFX_ENABLED); 
		m_lifx_enabled.SetCheck(cfg_lifx_enabled);
		
		SetDlgItemInt(IDC_LIFX_CYCLE_SPEED, cfg_lifx_cycle_speed, FALSE);
		SetDlgItemInt(IDC_LIFX_BRIGHTNESS, cfg_lifx_brightness, FALSE);

		return FALSE;
	}

	void OnEditChange(UINT, int, CWindow) {
		OnChanged();
	}

	bool HasChanged() {
		return m_lifx_enabled.GetCheck() != cfg_lifx_enabled || GetDlgItemInt(IDC_LIFX_CYCLE_SPEED, NULL, FALSE) != cfg_lifx_cycle_speed || GetDlgItemInt(IDC_LIFX_BRIGHTNESS) != cfg_lifx_brightness;
	}

	void OnChanged() {
		m_callback->on_state_changed();
	}
public:
	CLifxPreferences(preferences_page_callback::ptr callback): 
		m_callback(callback) {}

	//dialog resource ID
	enum { IDD = IDD_LIFX_FORMVIEW };
	
	t_uint32 get_state() {
		t_uint32 state = preferences_state::resettable;
		if (HasChanged()) state |= preferences_state::changed;
		return state;
	}

	void apply() {
		cfg_lifx_enabled = m_lifx_enabled.GetCheck();
		cfg_lifx_cycle_speed = GetDlgItemInt(IDC_LIFX_CYCLE_SPEED, NULL, FALSE);
		cfg_lifx_brightness = min(100, GetDlgItemInt(IDC_LIFX_BRIGHTNESS, NULL, FALSE));
		
		OnChanged();
	}

	void reset() {
		m_lifx_enabled.SetCheck(true);
		SetDlgItemInt(IDC_LIFX_CYCLE_SPEED, 30, FALSE);
		SetDlgItemInt(IDC_LIFX_BRIGHTNESS, 100, FALSE);

		OnChanged();
	}

	//WTL message map
	BEGIN_MSG_MAP(CLifxPreferences)
		MSG_WM_INITDIALOG(OnInitDialog)
		COMMAND_HANDLER_EX(IDC_LIFX_ENABLED, BN_CLICKED, OnEditChange)
		COMMAND_HANDLER_EX(IDC_LIFX_CYCLE_SPEED, EN_CHANGE, OnEditChange)
		COMMAND_HANDLER_EX(IDC_LIFX_BRIGHTNESS, EN_CHANGE, OnEditChange)
	END_MSG_MAP()
};

class preferences_page_lifx : public preferences_page_impl<CLifxPreferences> {
	// preferences_page_impl<> helper deals with instantiation of our dialog; inherits from preferences_page_v3.
public:
	const char * get_name() { return "Lifx"; }
	GUID get_guid() {
		// This is our GUID. Replace with your own when reusing the code.
		static const GUID guid = { 0xc34a7ae3, 0x30c2, 0x4071,{ 0xb9, 0x49, 0x1d, 0xbd, 0x12, 0x4a, 0x36, 0xfb } };
		return guid;
	}
	GUID get_parent_guid() { return guid_root; }
};

static preferences_page_factory_t<preferences_page_lifx> g_preferences_page_lifx_factory;