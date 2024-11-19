////////////////////////////////////////////////////////////////////////////////
//
// File: virtualLego.cpp
//
// Original Author: ��â�� Chang-hyeon Park, 
// Modified by Bong-Soo Sohn and Dong-Jun Kim
// 
// Originally programmed for Virtual LEGO. 
// Modified later to program for Virtual Billiard.
//        
////////////////////////////////////////////////////////////////////////////////

#include "d3dUtility.h"
#include <vector>
#include <ctime>
#include <cstdlib>
#include <cstdio>
#include <cassert>

IDirect3DDevice9* Device = NULL;

// window size
const int Width = 1024;
const int Height = 768;

// 10���� ��� ���� ��ġ �ʱ�ȭ(�߾ӿ� ���߾� ��ġ, �� ���� 0.42 �̻�)
const float spherePos[10][2] = {  // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! �迭ũ��� ��ġ ����
	{-3.5f, 2.4f}, {-2.5f, 2.4f}, {-1.5f, 2.4f}, {-0.5f, 2.4f}, {0.5f, 2.4f},
	{1.5f, 2.4f}, {2.5f, 2.4f}, {3.5f, 2.4f}, {-3.5f, 1.4f}, {3.5f, 1.4f}
};
// 10���� �� ��� �����
const D3DXCOLOR sphereColor[10] = {  // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! �� ����
	d3d::YELLOW, d3d::YELLOW, d3d::YELLOW, d3d::YELLOW, d3d::YELLOW,
	d3d::YELLOW, d3d::YELLOW, d3d::YELLOW, d3d::YELLOW, d3d::YELLOW
};

// -----------------------------------------------------------------------------
// Transform matrices
// -----------------------------------------------------------------------------
D3DXMATRIX g_mWorld;
D3DXMATRIX g_mView;
D3DXMATRIX g_mProj;

#define M_RADIUS 0.21   // ball radius
#define PI 3.14159265
#define M_HEIGHT 0.01
#define DECREASE_RATE 0.9982

// -----------------------------------------------------------------------------
// CSphere class definition
// -----------------------------------------------------------------------------

class CSphere {
private:
	float					center_x, center_y, center_z;
	float                   m_radius;
	float					m_velocity_x;
	float					m_velocity_z;

public:
	CSphere(void)
	{
		D3DXMatrixIdentity(&m_mLocal);
		ZeroMemory(&m_mtrl, sizeof(m_mtrl));
		m_radius = 0;
		m_velocity_x = 0;
		m_velocity_z = 0;
		m_pSphereMesh = NULL;
	}
	~CSphere(void) {}

public:
	bool create(IDirect3DDevice9* pDevice, D3DXCOLOR color = d3d::WHITE)
	{
		if (NULL == pDevice)
			return false;

		m_mtrl.Ambient = color;
		m_mtrl.Diffuse = color;
		m_mtrl.Specular = color;
		m_mtrl.Emissive = d3d::BLACK;
		m_mtrl.Power = 5.0f;

		if (FAILED(D3DXCreateSphere(pDevice, getRadius(), 50, 50, &m_pSphereMesh, NULL)))
			return false;
		return true;
	}

	void destroy(void)
	{
		if (m_pSphereMesh != NULL) {
			m_pSphereMesh->Release();
			m_pSphereMesh = NULL;
		}
	}

	void draw(IDirect3DDevice9* pDevice, const D3DXMATRIX& mWorld)
	{
		if (NULL == pDevice)
			return;
		pDevice->SetTransform(D3DTS_WORLD, &mWorld);
		pDevice->MultiplyTransform(D3DTS_WORLD, &m_mLocal);
		pDevice->SetMaterial(&m_mtrl);
		m_pSphereMesh->DrawSubset(0);
	}

	bool hasIntersected(CSphere& ball)
	{
		D3DXVECTOR3 center1 = this->getCenter();
		D3DXVECTOR3 center2 = ball.getCenter();
		float distance = D3DXVec3Length(&(center1 - center2)); // �� ���� �߽� ������ �Ÿ�
		return distance <= (this->getRadius() + ball.getRadius()); // �� ���� ������ �հ� ��
	}


	void hitBy(CSphere& ball)
	{
		if (hasIntersected(ball)) {
			// �� ���� �߽� ��ǥ
			D3DXVECTOR3 center1 = this->getCenter();
			D3DXVECTOR3 center2 = ball.getCenter();

			// �浹 ���� ���� ��� (��1 -> ��2)
			D3DXVECTOR3 collisionNormal = center2 - center1;
			float overlap = this->getRadius() + ball.getRadius() - D3DXVec3Length(&collisionNormal);

			if (overlap > 0.0f) {
				// ��ħ�� ���� (�� ���� �ݴ� �������� ��ħ�� ���ݸ�ŭ �̵�)
				D3DXVec3Normalize(&collisionNormal, &collisionNormal);
				this->setCenter(center1.x - collisionNormal.x * overlap * 0.5f,
					center1.y,
					center1.z - collisionNormal.z * overlap * 0.5f);
				ball.setCenter(center2.x + collisionNormal.x * overlap * 0.5f,
					center2.y,
					center2.z + collisionNormal.z * overlap * 0.5f);
			}

			// ���� �ӵ� ����
			D3DXVECTOR3 velocity1(this->getVelocity_X(), 0, this->getVelocity_Z());
			D3DXVECTOR3 velocity2(ball.getVelocity_X(), 0, ball.getVelocity_Z());

			// �浹 ���� ���Ϳ� ���� �ӵ� ������ ���� ���
			float v1_dot_n = D3DXVec3Dot(&velocity1, &collisionNormal);
			float v2_dot_n = D3DXVec3Dot(&velocity2, &collisionNormal);

			// ���ο� �ӵ� ��� (�ݻ� ����)
			D3DXVECTOR3 newVelocity1 = velocity1 - 2 * v1_dot_n * collisionNormal;
			D3DXVECTOR3 newVelocity2 = velocity2 - 2 * v2_dot_n * collisionNormal;

			// ���ο� �ӵ��� ���� ����
			this->setPower(newVelocity1.x, newVelocity1.z);
			ball.setPower(newVelocity2.x, newVelocity2.z);
		}
	}



	void ballUpdate(float timeDiff)
	{
		const float TIME_SCALE = 3.3;
		D3DXVECTOR3 cord = this->getCenter();
		double vx = abs(this->getVelocity_X());
		double vz = abs(this->getVelocity_Z());


		if (vx > 0.01 || vz > 0.01)
		{
			float tX = cord.x + TIME_SCALE * timeDiff * m_velocity_x;
			float tZ = cord.z + TIME_SCALE * timeDiff * m_velocity_z;

			//correction of position of ball
			// Please uncomment this part because this correction of ball position is necessary when a ball collides with a wall
			/*if(tX >= (4.5 - M_RADIUS))
				tX = 4.5 - M_RADIUS;
			else if(tX <=(-4.5 + M_RADIUS))
				tX = -4.5 + M_RADIUS;
			else if(tZ <= (-3 + M_RADIUS))
				tZ = -3 + M_RADIUS;
			else if(tZ >= (3 - M_RADIUS))
				tZ = 3 - M_RADIUS;*/

			this->setCenter(tX, cord.y, tZ);
		}
		else { this->setPower(0, 0); }
		//this->setPower(this->getVelocity_X() * DECREASE_RATE, this->getVelocity_Z() * DECREASE_RATE);
		double rate = 1 - (1 - DECREASE_RATE) * timeDiff * 400;
		if (rate < 0)
			rate = 0;
		this->setPower(getVelocity_X() * rate, getVelocity_Z() * rate);
	}

	double getVelocity_X() { return this->m_velocity_x; }
	double getVelocity_Z() { return this->m_velocity_z; }

	void setPower(double vx, double vz)
	{
		this->m_velocity_x = vx;
		this->m_velocity_z = vz;
	}

	void setCenter(float x, float y, float z)
	{
		D3DXMATRIX m;
		// ���� �߽� ��ǥ ������Ʈ
		center_x = x;
		center_y = y;
		center_z = z;
		// ��ġ ��� ������Ʈ
		D3DXMatrixTranslation(&m, x, y, z);
		setLocalTransform(m); // ���� ��ȯ ��� ����
	}

	float getRadius(void)  const { return (float)(M_RADIUS); }
	const D3DXMATRIX& getLocalTransform(void) const { return m_mLocal; }
	void setLocalTransform(const D3DXMATRIX& mLocal) { m_mLocal = mLocal; }
	D3DXVECTOR3 getCenter(void) const
	{
		D3DXVECTOR3 org(center_x, center_y, center_z);
		return org;
	}

private:
	D3DXMATRIX              m_mLocal;
	D3DMATERIAL9            m_mtrl;
	ID3DXMesh* m_pSphereMesh;

};



// -----------------------------------------------------------------------------
// CWall class definition
// -----------------------------------------------------------------------------

class CWall {

private:

	float					m_x;
	float					m_z;
	float                   m_width;
	float                   m_depth;
	float					m_height;

public:
	CWall(void)
	{
		D3DXMatrixIdentity(&m_mLocal);
		ZeroMemory(&m_mtrl, sizeof(m_mtrl));
		m_width = 0;
		m_depth = 0;
		m_pBoundMesh = NULL;
	}
	~CWall(void) {}
public:
	bool create(IDirect3DDevice9* pDevice, float ix, float iz, float iwidth, float iheight, float idepth, D3DXCOLOR color = d3d::WHITE)
	{
		if (NULL == pDevice)
			return false;

		m_mtrl.Ambient = color;
		m_mtrl.Diffuse = color;
		m_mtrl.Specular = color;
		m_mtrl.Emissive = d3d::BLACK;
		m_mtrl.Power = 5.0f;

		m_width = iwidth;
		m_depth = idepth;

		if (FAILED(D3DXCreateBox(pDevice, iwidth, iheight, idepth, &m_pBoundMesh, NULL)))
			return false;
		return true;
	}
	void destroy(void)
	{
		if (m_pBoundMesh != NULL) {
			m_pBoundMesh->Release();
			m_pBoundMesh = NULL;
		}
	}
	void draw(IDirect3DDevice9* pDevice, const D3DXMATRIX& mWorld)
	{
		if (NULL == pDevice)
			return;
		pDevice->SetTransform(D3DTS_WORLD, &mWorld);
		pDevice->MultiplyTransform(D3DTS_WORLD, &m_mLocal);
		pDevice->SetMaterial(&m_mtrl);
		m_pBoundMesh->DrawSubset(0);
	}

	bool hasIntersected(CSphere& ball)
	{
		// Insert your code here.
		return false;
	}

	void hitBy(CSphere& ball)
	{
		// Insert your code here.
	}

	void setPosition(float x, float y, float z)
	{
		D3DXMATRIX m;
		this->m_x = x;
		this->m_z = z;

		D3DXMatrixTranslation(&m, x, y, z);
		setLocalTransform(m);
	}

	float getHeight(void) const { return M_HEIGHT; }



private:
	void setLocalTransform(const D3DXMATRIX& mLocal) { m_mLocal = mLocal; }

	D3DXMATRIX              m_mLocal;
	D3DMATERIAL9            m_mtrl;
	ID3DXMesh* m_pBoundMesh;
};

// -----------------------------------------------------------------------------
// CLight class definition
// -----------------------------------------------------------------------------

class CLight {
public:
	CLight(void)
	{
		static DWORD i = 0;
		m_index = i++;
		D3DXMatrixIdentity(&m_mLocal);
		::ZeroMemory(&m_lit, sizeof(m_lit));
		m_pMesh = NULL;
		m_bound._center = D3DXVECTOR3(0.0f, 0.0f, 0.0f);
		m_bound._radius = 0.0f;
	}
	~CLight(void) {}
public:
	bool create(IDirect3DDevice9* pDevice, const D3DLIGHT9& lit, float radius = 0.1f)
	{
		if (NULL == pDevice)
			return false;
		if (FAILED(D3DXCreateSphere(pDevice, radius, 10, 10, &m_pMesh, NULL)))
			return false;

		m_bound._center = lit.Position;
		m_bound._radius = radius;

		m_lit.Type = lit.Type;
		m_lit.Diffuse = lit.Diffuse;
		m_lit.Specular = lit.Specular;
		m_lit.Ambient = lit.Ambient;
		m_lit.Position = lit.Position;
		m_lit.Direction = lit.Direction;
		m_lit.Range = lit.Range;
		m_lit.Falloff = lit.Falloff;
		m_lit.Attenuation0 = lit.Attenuation0;
		m_lit.Attenuation1 = lit.Attenuation1;
		m_lit.Attenuation2 = lit.Attenuation2;
		m_lit.Theta = lit.Theta;
		m_lit.Phi = lit.Phi;
		return true;
	}
	void destroy(void)
	{
		if (m_pMesh != NULL) {
			m_pMesh->Release();
			m_pMesh = NULL;
		}
	}
	bool setLight(IDirect3DDevice9* pDevice, const D3DXMATRIX& mWorld)
	{
		if (NULL == pDevice)
			return false;

		D3DXVECTOR3 pos(m_bound._center);
		D3DXVec3TransformCoord(&pos, &pos, &m_mLocal);
		D3DXVec3TransformCoord(&pos, &pos, &mWorld);
		m_lit.Position = pos;

		pDevice->SetLight(m_index, &m_lit);
		pDevice->LightEnable(m_index, TRUE);
		return true;
	}

	void draw(IDirect3DDevice9* pDevice)
	{
		if (NULL == pDevice)
			return;
		D3DXMATRIX m;
		D3DXMatrixTranslation(&m, m_lit.Position.x, m_lit.Position.y, m_lit.Position.z);
		pDevice->SetTransform(D3DTS_WORLD, &m);
		pDevice->SetMaterial(&d3d::WHITE_MTRL);
		m_pMesh->DrawSubset(0);
	}

	D3DXVECTOR3 getPosition(void) const { return D3DXVECTOR3(m_lit.Position); }

private:
	DWORD               m_index;
	D3DXMATRIX          m_mLocal;
	D3DLIGHT9           m_lit;
	ID3DXMesh* m_pMesh;
	d3d::BoundingSphere m_bound;
};


// -----------------------------------------------------------------------------
// Global variables
// -----------------------------------------------------------------------------
CWall	g_legoPlane;
CWall	g_legowall[4];
CSphere	g_sphere[10];
CSphere g_move;
CSphere	g_target_white;
CLight	g_light;
bool gameStarted = false;   // ���� ���� ����
bool ballLaunched = false; // ���� �� �߻� ����

double g_camera_pos[3] = { 0.0, 5.0, -8.0 };

// -----------------------------------------------------------------------------
// Functions
// -----------------------------------------------------------------------------


void destroyAllLegoBlock(void)
{
}

// initialization
bool Setup()
{
	int i;

	D3DXMatrixIdentity(&g_mWorld);
	D3DXMatrixIdentity(&g_mView);
	D3DXMatrixIdentity(&g_mProj);

	// create plane and set the position
	if (false == g_legoPlane.create(Device, -1, -1, 6, 0.03f, 9, d3d::GREEN)) return false;
	g_legoPlane.setPosition(0.0f, -0.0006f / 5, 0.0f);

	// create walls and set the position. note that there are four walls
	if (false == g_legowall[0].create(Device, -1, -1, 9, 0.3f, 0.12f, d3d::DARKRED)) return false;
	g_legowall[0].setPosition(0.0f, 0.12f, 3.06f);
	if (false == g_legowall[1].create(Device, -1, -1, 9, 0.3f, 0.12f, d3d::DARKRED)) return false;
	g_legowall[1].setPosition(0.0f, 0.12f, -3.06f);
	if (false == g_legowall[2].create(Device, -1, -1, 0.12f, 0.3f, 6.24f, d3d::DARKRED)) return false;
	g_legowall[2].setPosition(4.56f, 0.12f, 0.0f);
	if (false == g_legowall[3].create(Device, -1, -1, 0.12f, 0.3f, 6.24f, d3d::DARKRED)) return false;
	g_legowall[3].setPosition(-4.56f, 0.12f, 0.0f);

	// create four balls and set the position
	for (i = 0; i < 10; i++) {
		if (false == g_sphere[i].create(Device, sphereColor[i])) return false;
		g_sphere[i].setCenter(spherePos[i][0], (float)M_RADIUS, spherePos[i][1]);
		g_sphere[i].setPower(0, 0);
	}

	// create white ball for set direction
	if (false == g_target_white.create(Device, d3d::WHITE)) return false;
	g_target_white.setCenter(.0f, (float)M_RADIUS, -2.5f);

	//create move ball for set direction
	if (false == g_move.create(Device, d3d::RED)) return false;
	g_move.setCenter(.0f, (float)M_RADIUS, -2.5f + M_RADIUS);

	// light setting 
	D3DLIGHT9 lit;
	::ZeroMemory(&lit, sizeof(lit));
	lit.Type = D3DLIGHT_POINT;
	lit.Diffuse = d3d::WHITE;
	lit.Specular = d3d::WHITE * 0.9f;
	lit.Ambient = d3d::WHITE * 0.9f;
	lit.Position = D3DXVECTOR3(0.0f, 3.0f, 0.0f);
	lit.Range = 100.0f;
	lit.Attenuation0 = 0.0f;
	lit.Attenuation1 = 0.9f;
	lit.Attenuation2 = 0.0f;
	if (false == g_light.create(Device, lit))
		return false;

	// Position and aim the camera.
	D3DXVECTOR3 pos(0.0f, 5.0f, -8.0f);
	D3DXVECTOR3 target(0.0f, 0.0f, 0.0f);
	D3DXVECTOR3 up(0.0f, 2.0f, 0.0f);
	D3DXMatrixLookAtLH(&g_mView, &pos, &target, &up);
	Device->SetTransform(D3DTS_VIEW, &g_mView);

	// Set the projection matrix.
	D3DXMatrixPerspectiveFovLH(&g_mProj, D3DX_PI / 4,
		(float)Width / (float)Height, 1.0f, 100.0f);
	Device->SetTransform(D3DTS_PROJECTION, &g_mProj);

	// Set render states.
	Device->SetRenderState(D3DRS_LIGHTING, TRUE);
	Device->SetRenderState(D3DRS_SPECULARENABLE, TRUE);
	Device->SetRenderState(D3DRS_SHADEMODE, D3DSHADE_GOURAUD);

	g_light.setLight(Device, g_mWorld);
	return true;
}

void Cleanup(void)
{
	g_legoPlane.destroy();
	for (int i = 0; i < 4; i++) {
		g_legowall[i].destroy();
	}
	destroyAllLegoBlock();
	g_light.destroy();
}


// timeDelta represents the time between the current image frame and the last image frame.
// the distance of moving balls should be "velocity * timeDelta"
bool Display(float timeDelta)
{
	int i = 0;
	int j = 0;


	if (Device)
	{
		Device->Clear(0, 0, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, 0x00afafaf, 1.0f, 0);
		Device->BeginScene();

		// update the position of each ball. during update, check whether each ball hit by walls.
		for (i = 0; i < 10; i++) {
			g_sphere[i].ballUpdate(timeDelta);
			for (j = 0; j < 10; j++) { g_legowall[i].hitBy(g_sphere[j]); }
		}

		// check whether any two balls hit together and update the direction of balls
		for (i = 0; i < 10; i++) {
			for (j = 0; j < 10; j++) {
				if (i >= j) { continue; }
				g_sphere[i].hitBy(g_sphere[j]);
			}
		}
		g_target_white.hitBy(g_move);
		// Additional collision check with the target blue ball
		for (i = 0; i < 10; i++) {
			g_sphere[i].hitBy(g_move);  // �� ���� �Ķ� ���� �浹�ϴ��� Ȯ��
		}
		// draw plane, walls, and spheres
		g_legoPlane.draw(Device, g_mWorld);
		for (i = 0; i < 4; i++) {
			g_legowall[i].draw(Device, g_mWorld);
		}
		// Display �Լ����� ������ �κ�
		for (i = 0; i < 10; i++) { // ��� �� 10�� ��� �׸���
			g_sphere[i].draw(Device, g_mWorld);
		}
		//�Ͼ� �� ������Ʈ
		g_target_white.ballUpdate(timeDelta);
		g_target_white.draw(Device, g_mWorld);
		// ���� �� ������Ʈ
		if (ballLaunched) {
			// ���� �� ������Ʈ (�߻� ��)
			g_move.ballUpdate(timeDelta);
		}
		g_move.draw(Device, g_mWorld);
		g_light.draw(Device);

		Device->EndScene();
		Device->Present(0, 0, 0, 0);
		Device->SetTexture(0, NULL);
	}
	return true;
}

LRESULT CALLBACK d3d::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	static bool wire = false;
	static bool isReset = true;
	static int old_x = 0;
	static int old_y = 0;
	static enum { WORLD_MOVE, LIGHT_MOVE, BLOCK_MOVE } move = WORLD_MOVE;

	switch (msg) {
	case WM_DESTROY:
		::PostQuitMessage(0);
		break;

	case WM_KEYDOWN: {
		switch (wParam) {
		case VK_ESCAPE:
			::DestroyWindow(hwnd);
			break;

		case VK_SPACE: {
			if (!gameStarted) {
				// ù ��° �����̽��� �Է�: ���� ����
				gameStarted = true;
				ballLaunched = false;       // ���� ���� �߻���� ����
				g_move.setPower(0.0f, 0.0f); // ���� �� ���� ���� ����
			}

			if (!ballLaunched) {
				// �� �߻� ó��
				ballLaunched = true;

				// �� ���� ��ġ�� ���� ���� ��ġ�� �������� ���� ���� ���
				D3DXVECTOR3 redPos = g_move.getCenter();
				D3DXVECTOR3 whitePos = g_target_white.getCenter();
				D3DXVECTOR3 direction = redPos - whitePos; // �� ���� �ݴ� ���� ���
				D3DXVec3Normalize(&direction, &direction);

				// �ӵ��� ���� ���Ϳ� ����
				g_move.setPower(direction.x * 5.0f, direction.z * 5.0f);
			}
			break;
		}


		case VK_LEFT: {
			if (!gameStarted || !ballLaunched) {
				// ���� ���� ��: ���� ���� �� �� ���� ������
				D3DXVECTOR3 redPos = g_move.getCenter();
				D3DXVECTOR3 whitePos = g_target_white.getCenter();
				g_move.setCenter(redPos.x - 0.2f, redPos.y, redPos.z);
				g_target_white.setCenter(whitePos.x - 0.2f, whitePos.y, whitePos.z);
			}
			else if (gameStarted && ballLaunched) {
				// ���� ���� ��: �� ���� ������
				D3DXVECTOR3 whitePos = g_target_white.getCenter();
				g_target_white.setCenter(whitePos.x - 0.2f, whitePos.y, whitePos.z);
			}
			break;
		}

		case VK_RIGHT: {
			if (!gameStarted || !ballLaunched) {
				// ���� ���� ��: ���� ���� �� �� ���� ������
				D3DXVECTOR3 redPos = g_move.getCenter();
				D3DXVECTOR3 whitePos = g_target_white.getCenter();
				g_move.setCenter(redPos.x + 0.2f, redPos.y, redPos.z);
				g_target_white.setCenter(whitePos.x + 0.2f, whitePos.y, whitePos.z);
			}
			else if (gameStarted && ballLaunched) {
				// ���� ���� ��: �� ���� ������
				D3DXVECTOR3 whitePos = g_target_white.getCenter();
				g_target_white.setCenter(whitePos.x + 0.2f, whitePos.y, whitePos.z);
			}
			break;
		}
		}
		break;
	}



	default:
		return ::DefWindowProc(hwnd, msg, wParam, lParam);
	} // �ܺ� switch(msg)�� ��

	return 0;
}



int WINAPI WinMain(HINSTANCE hinstance,
	HINSTANCE prevInstance,
	PSTR cmdLine,
	int showCmd)
{
	srand(static_cast<unsigned int>(time(NULL)));

	if (!d3d::InitD3D(hinstance,
		Width, Height, true, D3DDEVTYPE_HAL, &Device))
	{
		::MessageBox(0, "InitD3D() - FAILED", 0, 0);
		return 0;
	}

	if (!Setup())
	{
		::MessageBox(0, "Setup() - FAILED", 0, 0);
		return 0;
	}

	d3d::EnterMsgLoop(Display);

	Cleanup();

	Device->Release();

	return 0;
}