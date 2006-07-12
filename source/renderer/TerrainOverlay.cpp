#include "precompiled.h"

#include "TerrainOverlay.h"

#include "ps/Game.h"
#include "ps/World.h"
#include "graphics/Terrain.h"
#include "maths/MathUtil.h"
#include "lib/ogl.h"

#include <algorithm>

// Handy things for STL:

/// Functor for sorting pairs, using the &lt;-ordering of their second values.
struct compare2nd
{
	template<typename S, typename T> bool operator()(const std::pair<S, T>& a, const std::pair<S, T>& b) const
	{
		return a.second < b.second;
	}
};

/// Functor for comparing the firsts of pairs to a specified value.
template<typename S> struct equal1st
{
	const S& val;
	equal1st(const S& val) : val(val) {}
	template <typename T> bool operator()(const std::pair<S, T>& a) const
	{
		return a.first == val;
	}
private:
	const equal1st& operator=(const equal1st& rhs);
};

/// Functor for calling ->Render on pairs' firsts.
struct render1st
{
	template<typename S, typename T> void operator()(const std::pair<S, T>& a) const
	{
		a.first->Render();
	}
};

//////////////////////////////////////////////////////////////////////////

// Global overlay list management:

static std::vector<std::pair<TerrainOverlay*, int> > g_TerrainOverlayList;

TerrainOverlay::TerrainOverlay(int priority)
{
	// Add to global list of overlays
	g_TerrainOverlayList.push_back(std::make_pair(this, priority));
	// Sort by overlays by priority. Do stable sort so that adding/removing
	// overlays doesn't randomly disturb all the existing ones (which would
	// be noticeable if they have the same priority and overlap).
	std::stable_sort(g_TerrainOverlayList.begin(), g_TerrainOverlayList.end(),
		compare2nd());
}

TerrainOverlay::~TerrainOverlay()
{
	std::vector<std::pair<TerrainOverlay*, int> >::iterator newEnd =
		std::remove_if(g_TerrainOverlayList.begin(), g_TerrainOverlayList.end(),
			equal1st<TerrainOverlay*>(this));
	g_TerrainOverlayList.erase(newEnd, g_TerrainOverlayList.end());
}

void TerrainOverlay::RenderOverlays()
{
	if (g_TerrainOverlayList.size() == 0)
		return;

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDepthMask(GL_FALSE);
	// To ensure that outlines are drawn on top of the terrain correctly (and
	// don't Z-fight and flicker nastily), draw them as QUADS with the LINE
	// PolygonMode, and use PolygonOffset to pull them towards the camera.
	// (See e.g. http://www.opengl.org/resources/faq/technical/polygonoffset.htm)
	glPolygonOffset(-1.f, -1.f);
	glEnable(GL_POLYGON_OFFSET_LINE);

	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

	std::for_each(g_TerrainOverlayList.begin(), g_TerrainOverlayList.end(),
		render1st());

	// Clean up state changes
	glEnable(GL_CULL_FACE);
	glEnable(GL_DEPTH_TEST);
	glDisable(GL_POLYGON_OFFSET_LINE);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glDepthMask(GL_TRUE);
	glDisable(GL_BLEND);
}

//////////////////////////////////////////////////////////////////////////

void TerrainOverlay::GetTileExtents(
	int& min_i_inclusive, int& min_j_inclusive,
	int& max_i_inclusive, int& max_j_inclusive)
{
	// Default to whole map
	min_i_inclusive = min_j_inclusive = 0;
	max_i_inclusive = max_j_inclusive = m_Terrain->GetTilesPerSide()-1;
}

void TerrainOverlay::Render()
{
	m_Terrain = g_Game->GetWorld()->GetTerrain();

	int min_i, min_j, max_i, max_j;
	GetTileExtents(min_i, min_j, max_i, max_j);
	// Clamp the min to 0, but the max to -1 - so tile -1 can never be rendered,
	// but if unclamped_max<0 then no tiles at all will be rendered. And the same
	// for the upper limit.
	min_i = clamp(min_i, 0, (int)m_Terrain->GetTilesPerSide());
	min_j = clamp(min_j, 0, (int)m_Terrain->GetTilesPerSide());
	max_i = clamp(max_i, -1, (int)m_Terrain->GetTilesPerSide()-1);
	max_j = clamp(max_j, -1, (int)m_Terrain->GetTilesPerSide()-1);

	for (m_j = min_j; m_j <= max_j; ++m_j)
		for (m_i = min_i; m_i <= max_i; ++m_i)
			ProcessTile(m_i, m_j);
}

void TerrainOverlay::RenderTile(const CColor& colour, bool draw_hidden)
{
	// TODO: if this is unpleasantly slow, make it much more efficient
	// (e.g. buffering data and making a single draw call? or at least
	// far fewer calls than it makes now)

	if (draw_hidden)
	{
		glDisable(GL_DEPTH_TEST);
		glDisable(GL_CULL_FACE);
	}
	else
	{
		glEnable(GL_DEPTH_TEST);
		glEnable(GL_CULL_FACE);
	}
	
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

	CVector3D pos;
	glBegin(GL_QUADS);
		glColor4fv(colour.FloatArray());
		m_Terrain->CalcPosition(m_i,   m_j,   pos); glVertex3fv(pos.GetFloatArray());
		m_Terrain->CalcPosition(m_i+1, m_j,   pos); glVertex3fv(pos.GetFloatArray());
		m_Terrain->CalcPosition(m_i+1, m_j+1, pos); glVertex3fv(pos.GetFloatArray());
		m_Terrain->CalcPosition(m_i,   m_j+1, pos); glVertex3fv(pos.GetFloatArray());
	glEnd();
}

void TerrainOverlay::RenderTileOutline(const CColor& colour, int line_width, bool draw_hidden)
{
	if (draw_hidden)
	{
		glDisable(GL_DEPTH_TEST);
		glDisable(GL_CULL_FACE);
	}
	else
	{
		glEnable(GL_DEPTH_TEST);
		glEnable(GL_CULL_FACE);
	}

	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

	glLineWidth((float)line_width);

	CVector3D pos;
	glBegin(GL_QUADS);
		glColor4fv(colour.FloatArray());
		m_Terrain->CalcPosition(m_i,   m_j,   pos); glVertex3fv(pos.GetFloatArray());
		m_Terrain->CalcPosition(m_i+1, m_j,   pos); glVertex3fv(pos.GetFloatArray());
		m_Terrain->CalcPosition(m_i+1, m_j+1, pos); glVertex3fv(pos.GetFloatArray());
		m_Terrain->CalcPosition(m_i,   m_j+1, pos); glVertex3fv(pos.GetFloatArray());
	glEnd();
}
