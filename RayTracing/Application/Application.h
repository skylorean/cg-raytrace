﻿#pragma once
#include <SDL.h>
#include <memory>
#include "../FrameBuffer/FrameBuffer.h"
#include "../GeometryObjects/Plane/Plane.h"
#include "../RenderContext/RenderContext.h"
#include "../Renderer/Renderer.h"
#include "../Scene/Scene.h"
#include "../Shader/SimpleDiffuseShader.h"
#include "../Shader/PhongShader.h"
#include "../GeometryObjects/Cube/Cube.h"
#include "../TriangleMesh/TriangleMesh.h"

class Application
{
public:
	Application();

	~Application();

	void MainLoop();

private:
	void Initialize();

	void Uninitialize();

	// Обновление содержимого окна приложения
	void UpdateMainSurface();

	// Копирование буфера кадра в область главного окна
	void CopyFrameBufferToSDLSurface();

	// Обработчик таймера, вызываемый SDL
	static Uint32 SDLCALL TimerCallback(Uint32 interval, void* param);

	// Обработчик таймера SDL объекта CApplication
	Uint32 OnTimer(Uint32 interval);

	// Пометка содержимого окна, как нуждающейся в перерисовке
	void InvalidateMainSurface();

	void AddSomePlane();
	void AddSomeLight();
	void AddSomeCubes();
	void AddSomeTetrahedron();
	void AddSomeOctahedron();
	void AddSomeDodecahedron();
	void AddSomeIcosahedron();
	void AddSomeHyperbolicParaboloid();

	// Методы создания и добавления шейдеров в коллекцию m_shaders
	CSimpleDiffuseShader& CreateSimpleDiffuseShader(CSimpleMaterial const& material);
	PhongShader& CreatePhongShader(const ComplexMaterial& material);

	// Методы, создающие и добавляющие объекты к сцене
	CSceneObject& AddPlane(IShader const& shader, double a, double b, double c, double d, CMatrix4d const& transform = CMatrix4d());
	CSceneObject& AddSceneObject(IGeometryObject const& object, IShader const& shader);
	CSceneObject& AddCube(IShader const& shader, double size, CVector3d const& center = CVector3d(), CMatrix4d const& transform = CMatrix4d());
	CSceneObject& AddTriangleMesh(IShader const& shader, CTriangleMeshData const* pMeshData, CMatrix4d const& transform = CMatrix4d());
	CTriangleMeshData* CreateTriangleMeshData(std::vector<Vertex> const& vertices, std::vector<Face> const& faces);
	CSceneObject& AddDodecahedron(IShader const& shader, CMatrix4d const& transform = CMatrix4d());
	CSceneObject& AddIcosahedron(IShader const& shader, CMatrix4d const& transform = CMatrix4d());
	CSceneObject& AddHyperbolicParaboloid(IShader const& shader, CMatrix4d const& transform = CMatrix4d());

private:
	// Буфер кадра
	FrameBuffer m_frameBuffer;
	// Визуализатор
	Renderer m_renderer;
	// Сцена
	CScene m_scene;
	// Контекст
	CRenderContext m_context;

	// Поверхность окна приложения
	SDL_Surface* m_pMainSurface;
	// Идентификатор SDL-таймера
	SDL_TimerID m_timerId;
	// Обновлена ли поверхность окна приложения (1 - да, 0 - нет)
	std::atomic<uint32_t> m_mainSurfaceUpdated;

	std::vector<std::unique_ptr<IGeometryObject>> m_geometryObjects;
	std::vector<std::unique_ptr<IShader>> m_shaders;
	std::vector<std::unique_ptr<CTriangleMeshData>> m_triangleMeshDataObjects;
};
