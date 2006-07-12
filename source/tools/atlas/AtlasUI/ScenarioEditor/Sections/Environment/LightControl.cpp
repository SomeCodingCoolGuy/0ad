#include "stdafx.h"

#include "LightControl.h"

using AtlasMessage::Shareable;

class LightSphere : public wxControl
{
public:

	LightSphere(wxWindow* parent, LightControl* lightControl)
		: wxControl(parent, wxID_ANY, wxDefaultPosition, wxSize(150, 150)),
		m_LightControl(lightControl)
	{
	}

	void OnPaint(wxPaintEvent& event)
	{
		// Draw a lit 3D sphere:

		int w = GetClientSize().GetWidth();
		int h = GetClientSize().GetHeight();

		float lx = sin(-theta)*cos(phi);
		float ly = cos(-theta)*cos(phi);
		float lz = sin(phi);

		wxImage img (w, h);
		unsigned char* imgData = img.GetData();

		for (int y = 0; y < h; ++y)
		{
			for (int x = 0; x < w; ++x)
			{
				// Calculate normal vector of sphere (radius 1)
				float nx = 2*x / (float)(w-1) - 1; // [-1, 1]
				float ny = 2*y / (float)(h-1) - 1;
				float nz2 = 1 - nx*nx - ny*ny;
				if (nz2 >= 0.f)
				{
					float nz = sqrt(nz2);

					// Get reflection vector (r) of camera vector (c) in n
					float cx = 0;
					float cy = 0;
					float cz = 1; // (camera is infinitely far upwards)
					float ndotc = nz;
					float rx = -cx + nx*(2*ndotc);
					float ry = -cy + ny*(2*ndotc);
					float rz = -cz + nz*(2*ndotc);

					float ndotl = nx*lx + ny*ly + nz*lz;
					float rdotl = rx*lx + ry*ly + rz*lz;

					int diffuse = (int)std::max(0.f, ndotl*128.f);
					int specular = (int)std::min(255.f, 64.f*pow(std::max(0.f, rdotl), 16.f));

					imgData[0] = std::min(64+diffuse+specular, 255);
					imgData[1] = std::min(48+diffuse+specular, 255);
					imgData[2] = std::min(48+diffuse+specular, 255);
				}
				else
				{
					imgData[0] = 0;
					imgData[1] = 0;
					imgData[2] = 0;
				}
				imgData += 3;
			}
		}

		wxPaintDC dc(this);
		dc.DrawBitmap(wxBitmap(img, dc), 0, 0);
	}

	void OnMouse(wxMouseEvent& event)
	{
		if (event.LeftIsDown())
		{
			int x = event.GetX();
			int y = event.GetY();
			int w = GetClientSize().GetWidth();
			int h = GetClientSize().GetHeight();

			float mx = 2*x / (float)(w-1) - 1; // [-1, 1]
			float my = 2*y / (float)(h-1) - 1;
			float mz2 = 1 - mx*mx - my*my;
			if (mz2 >= 0.f)
			{
				float mz = sqrt(mz2);
				theta = -atan2(mx, my);
				phi = asin(mz);
			}
			else
			{
				theta = -atan2(mx, my);
				phi = 0;
			}

			Refresh(false);
			m_LightControl->NotifyOtherObservers();
		}
	}

	float theta, phi;
	LightControl* m_LightControl;

	DECLARE_EVENT_TABLE();
};

BEGIN_EVENT_TABLE(LightSphere, wxControl)
	EVT_PAINT(LightSphere::OnPaint)
	EVT_MOTION(LightSphere::OnMouse)
	EVT_LEFT_DOWN(LightSphere::OnMouse)
END_EVENT_TABLE()

LightControl::LightControl(wxWindow* parent, Observable<AtlasMessage::sEnvironmentSettings>& environment)
: wxPanel(parent), m_Environment(environment)
{
	m_Sphere = new LightSphere(this, this);
	m_Sphere->theta = environment.sunrotation;
	m_Sphere->phi = environment.sunelevation;

	m_Conn = environment.RegisterObserver(0, &LightControl::OnSettingsChange, this);
}

LightControl::~LightControl()
{
	m_Conn.disconnect();
}

void LightControl::OnSettingsChange(const AtlasMessage::sEnvironmentSettings& settings)
{
	m_Sphere->theta = settings.sunrotation;
	m_Sphere->phi = settings.sunelevation;
	m_Sphere->Refresh(false);
}

void LightControl::NotifyOtherObservers()
{
	m_Environment.sunrotation = m_Sphere->theta;
	m_Environment.sunelevation = m_Sphere->phi;
	m_Environment.NotifyObserversExcept(m_Conn);
}
