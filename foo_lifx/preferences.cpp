#include "stdafx.h"
#include "preferences.h"

cfg_bool cfg_lifx_enabled(guid_cfg_lifx_enabled, true);

cfg_bool cfg_lifx_cycle_enabled(guid_cfg_lifx_cycle_enabled, true);
cfg_uint cfg_lifx_cycle_speed(guid_cfg_lifx_cycle_speed, 30);

cfg_uint cfg_lifx_brightness(guid_cfg_lifx_brightness, 80);
cfg_uint cfg_lifx_hue(guid_cfg_lifx_hue, 0);
cfg_uint cfg_lifx_saturation(guid_cfg_lifx_saturation, 0);
cfg_uint cfg_lifx_intensity(guid_cfg_lifx_intensity, 0);
cfg_uint cfg_lifx_offset(guid_cfg_lifx_offset, 125);
cfg_uint cfg_lifx_delay(guid_cfg_lifx_delay, 66);

class CLifxPreferences : public CDialogImpl<CLifxPreferences>, public preferences_page_instance {
private:
	const preferences_page_callback::ptr m_callback;
	CCheckBox m_lifx_enabled;
	CCheckBox m_lifx_cycle_enabled;
	CComboBox m_lifx_intensity;
	CEdit m_lifx_cycle_speed;
	CEdit m_lifx_hue;
	CEdit m_lifx_saturation;
	
	void UpdateControls() {
		// Update the controls we need to disable depending on the state of colour cycling 
		if (m_lifx_cycle_enabled.GetCheck()) {
			m_lifx_cycle_speed.EnableWindow(TRUE);
			m_lifx_hue.EnableWindow(FALSE);
			m_lifx_saturation.EnableWindow(FALSE);
		}
		else {
			m_lifx_cycle_speed.EnableWindow(FALSE);
			m_lifx_hue.EnableWindow(TRUE);
			m_lifx_saturation.EnableWindow(TRUE);
		}
	}

	BOOL OnInitDialog(CWindow, LPARAM) {
		m_lifx_enabled = GetDlgItem(IDC_LIFX_ENABLED);
		m_lifx_enabled.SetCheck(cfg_lifx_enabled);

		m_lifx_cycle_enabled = GetDlgItem(IDC_LIFX_CYCLE_ENABLED);
		m_lifx_cycle_enabled.SetCheck(cfg_lifx_cycle_enabled);

		m_lifx_intensity = GetDlgItem(IDC_LIFX_INTENSITY);
		m_lifx_intensity.InsertString(0, L"Normal");
		m_lifx_intensity.InsertString(1, L"High");
		m_lifx_intensity.InsertString(2, L"Very High");
		m_lifx_intensity.InsertString(3, L"Maximum");
		m_lifx_intensity.SetCurSel(cfg_lifx_intensity);

		m_lifx_cycle_speed = GetDlgItem(IDC_LIFX_CYCLE_SPEED);
		m_lifx_hue = GetDlgItem(IDC_LIFX_HUE);
		m_lifx_saturation = GetDlgItem(IDC_LIFX_SATURATION);
		
		SetDlgItemInt(IDC_LIFX_CYCLE_SPEED, cfg_lifx_cycle_speed, FALSE);
		SetDlgItemInt(IDC_LIFX_BRIGHTNESS, cfg_lifx_brightness, FALSE);
		SetDlgItemInt(IDC_LIFX_OFFSET, cfg_lifx_offset, FALSE);
		SetDlgItemInt(IDC_LIFX_DELAY, cfg_lifx_delay, FALSE);
		SetDlgItemInt(IDC_LIFX_HUE, cfg_lifx_hue, FALSE);
		SetDlgItemInt(IDC_LIFX_SATURATION, cfg_lifx_saturation, FALSE);

		UpdateControls();

		return FALSE;
	}

	void OnEditChange(UINT, int, CWindow) {
		OnChanged();
	}

	bool HasChanged() {
		return	
			m_lifx_enabled.GetCheck() != cfg_lifx_enabled || 
			m_lifx_cycle_enabled.GetCheck() != cfg_lifx_cycle_enabled || 
			m_lifx_intensity.GetCurSel() != cfg_lifx_intensity || 
			GetDlgItemInt(IDC_LIFX_CYCLE_SPEED, NULL, FALSE) != cfg_lifx_cycle_speed || 
			GetDlgItemInt(IDC_LIFX_BRIGHTNESS) != cfg_lifx_brightness ||
			GetDlgItemInt(IDC_LIFX_OFFSET) != cfg_lifx_offset ||
			GetDlgItemInt(IDC_LIFX_DELAY) != cfg_lifx_delay ||
			GetDlgItemInt(IDC_LIFX_HUE) != cfg_lifx_hue ||
			GetDlgItemInt(IDC_LIFX_SATURATION) != cfg_lifx_saturation;
	}

	void OnChanged() {
		m_callback->on_state_changed();
		UpdateControls();
	}
public:
	CLifxPreferences(preferences_page_callback::ptr callback): 
		m_callback(callback) {}

	enum { IDD = IDD_LIFX_FORMVIEW };
	
	t_uint32 get_state() {
		t_uint32 state = preferences_state::resettable;
		if (HasChanged()) state |= preferences_state::changed;
		return state;
	}

	void apply() {
		cfg_lifx_enabled = m_lifx_enabled.GetCheck();

		cfg_lifx_cycle_enabled = m_lifx_cycle_enabled.GetCheck();
		cfg_lifx_cycle_speed = GetDlgItemInt(IDC_LIFX_CYCLE_SPEED, NULL, FALSE);
		
		cfg_lifx_intensity = m_lifx_intensity.GetCurSel();

		cfg_lifx_brightness = min(100, GetDlgItemInt(IDC_LIFX_BRIGHTNESS, NULL, FALSE));
		cfg_lifx_offset = max(50, GetDlgItemInt(IDC_LIFX_OFFSET, NULL, FALSE));
		cfg_lifx_delay = min(1000, GetDlgItemInt(IDC_LIFX_DELAY, NULL, FALSE));
		cfg_lifx_hue = min(360, max(0, GetDlgItemInt(IDC_LIFX_HUE, NULL, FALSE)));
		cfg_lifx_saturation = min(100, max(0, GetDlgItemInt(IDC_LIFX_SATURATION, NULL, FALSE)));

		OnChanged();
	}

	void reset() {
		m_lifx_enabled.SetCheck(true);

		m_lifx_cycle_enabled.SetCheck(true);
		SetDlgItemInt(IDC_LIFX_CYCLE_SPEED, 30, FALSE);
		
		m_lifx_intensity.SetCurSel(0);

		SetDlgItemInt(IDC_LIFX_BRIGHTNESS, 80, FALSE);
		SetDlgItemInt(IDC_LIFX_HUE, 0, FALSE);
		SetDlgItemInt(IDC_LIFX_SATURATION, 0, FALSE);

		OnChanged();
	}

	//WTL message map
	BEGIN_MSG_MAP(CLifxPreferences)
		MSG_WM_INITDIALOG(OnInitDialog)
		COMMAND_HANDLER_EX(IDC_LIFX_ENABLED, BN_CLICKED, OnEditChange)
		COMMAND_HANDLER_EX(IDC_LIFX_CYCLE_ENABLED, BN_CLICKED, OnEditChange)
		COMMAND_HANDLER_EX(IDC_LIFX_CYCLE_SPEED, EN_CHANGE, OnEditChange)
		COMMAND_HANDLER_EX(IDC_LIFX_BRIGHTNESS, EN_CHANGE, OnEditChange)
		COMMAND_HANDLER_EX(IDC_LIFX_OFFSET, EN_CHANGE, OnEditChange)
		COMMAND_HANDLER_EX(IDC_LIFX_DELAY, EN_CHANGE, OnEditChange)
		COMMAND_HANDLER_EX(IDC_LIFX_HUE, EN_CHANGE, OnEditChange)
		COMMAND_HANDLER_EX(IDC_LIFX_SATURATION, EN_CHANGE, OnEditChange)
		COMMAND_HANDLER_EX(IDC_LIFX_INTENSITY, CBN_SELCHANGE, OnEditChange)
	END_MSG_MAP()
};

class preferences_page_lifx : public preferences_page_impl<CLifxPreferences> {
public:
	const char * get_name() { return "Lifx"; }
	GUID get_guid() {
		static const GUID guid = { 0xc34a7ae3, 0x30c2, 0x4071,{ 0xb9, 0x49, 0x1d, 0xbd, 0x12, 0x4a, 0x36, 0xfb } };
		return guid;
	}
	GUID get_parent_guid() { return guid_root; }
};

static preferences_page_factory_t<preferences_page_lifx> g_preferences_page_lifx_factory;