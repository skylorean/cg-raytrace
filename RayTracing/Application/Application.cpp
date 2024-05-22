// Debug
#include <iostream>

#include "Application.h"
#include "../LightSource/OmniLightSource.h"
#include "../SceneObject/SceneObject.h"
#include "../Vector/VectorMath.h"
#include "../ViewPort/ViewPort.h"
#include "../Material/ComplexMaterial.h"
#include "../TriangleMesh/TriangleMesh.h"
#include "../GeometryObjects/Dodecahedron/Dodecahedron.h"
#include "../GeometryObjects/Icosahedron/Icosahedron.h"
#include "../GeometryObjects/HyperbolicParaboloid/HyperbolicParaboloid.h"


// Debug
size_t MOVABLE_LIGHT_SOURCE_INDEX = 0;

Application::Application()
	: m_frameBuffer(600, 400)
	, m_pMainSurface(NULL)
	, m_timerId(NULL)
	, m_mainSurfaceUpdated(0)
{
	// Инициализация SDL (таймер и видео)
	SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER);
	SDL_WM_SetCaption("Raytracing example", NULL);

	// Создаем главное окно приложения и сохраняем указатель
	// на поверхность, связанную с ним
	m_pMainSurface = SDL_SetVideoMode(600, 400, 32,
		SDL_SWSURFACE | SDL_DOUBLEBUF);

	/*
	Задаем цвет заднего фона сцены
	*/
	m_scene.SetBackdropColor(CVector4f(0, 0, 1, 1));

	AddSomePlane();
	AddSomeLight();

	AddSomeHyperbolicParaboloid();
	AddSomeCubes();
	AddSomeTetrahedron();
	AddSomeOctahedron();
	AddSomeDodecahedron();
	AddSomeIcosahedron();

	/*
		Задаем параметры видового порта и матрицы проецирования в контексте визуализации
	*/
	m_context.SetViewPort(CViewPort(0, 0, 600, 400));
	CMatrix4d proj;
	proj.LoadPerspective(75, 600.0 / 400.0, 0.1, 10);
	m_context.SetProjectionMatrix(proj);

	// Задаем матрицу камеры
	CMatrix4d modelView;
	modelView.LoadLookAtRH(
		1, 1, 5,
		0, 0, 0,
		0, 1, 0);
	m_context.SetModelViewMatrix(modelView);
}

Application::~Application()
{
	// Завершаем работу всех подсистем SDL
	SDL_Quit();
}

void Application::MainLoop()
{
	// Инициализация приложения.
	// Запуск в отдельном потоке и запуск таймера для периодического обновления содержимого окна приложения.
	Initialize();

	// Обновляем изначальное содержимое окна
	UpdateMainSurface();

	// Цикл обработки сообщений, продолжающийся пока не будет
	// получен запрос на завершение работы
	SDL_Event evt;
	while (SDL_WaitEvent(&evt) && evt.type != SDL_QUIT)
	{
		switch (evt.type)
		{
		case SDL_VIDEOEXPOSE:
		{
			// Обновляем содержимое главного окна,
			// если оно нуждается в перерисовке
			UpdateMainSurface();
			break;
		}
		case SDL_KEYDOWN:
		{
			CMatrix4d modelViewMatrix = m_context.GetModelViewMatrix();
			auto lightTranslate = m_scene.GetLight(MOVABLE_LIGHT_SOURCE_INDEX).GetTransform();
			bool cameraPosChanged = false;
			bool lightPosChanged = false;
			switch (evt.key.keysym.sym)
			{
			case SDLK_UP:
				lightTranslate.Translate(0, 1, 0);
				lightPosChanged = true;
				break;
			case SDLK_DOWN:
				lightTranslate.Translate(0, -1, 0);
				lightPosChanged = true;
				break;
			case SDLK_LEFT:
				lightTranslate.Translate(-1, 0, 0);
				lightPosChanged = true;
				break;
			case SDLK_RIGHT:
				lightTranslate.Translate(1, 0, 0);
				lightPosChanged = true;
				break;
			default:
				break;
			}
			if (lightPosChanged)
			{
				m_scene.GetLight(MOVABLE_LIGHT_SOURCE_INDEX).SetTransform(lightTranslate);
				Initialize();
			}
		}
		}
	};

	// Деинициализация приложения
	// Остановка таймера и процесса построения изображения, если он ещё не завершился
	Uninitialize();
}

void Application::Initialize()
{
	// Запускаем построение изображения и таймер обновления экрана
	m_renderer.Render(m_scene, m_context, m_frameBuffer);
	m_timerId = SDL_AddTimer(50, &TimerCallback, this);
}

void Application::Uninitialize()
{
	// Останавливаем таймер обновления экрана и построение изображения
	SDL_RemoveTimer(m_timerId);
	m_renderer.Stop();
}

// Обновляем содержимое главного окна

// Основной поток приложения выполняет периодическое обновление окна. 
// Копируя в его теневой (внеэкранный буфер) содержимое буфера кадра.

// То есть крч: подсистема визуализации -> Буфер кадра -> Теневой буфер, связанный с окном -> Видимое изображение в окне
void Application::UpdateMainSurface()
{
	// Копирование буфера кадра в область главного окна
	CopyFrameBufferToSDLSurface(); // (1)

	// Для отображения содержимого на экране из внеэкранного буфера.
	SDL_Flip(m_pMainSurface); // (2)

	// Устанавливается флаг "Поверхность обновлена"
	m_mainSurfaceUpdated.store(1); // (3)
}

// Копирование буфера кадра в область главного окна
// Перенос пикселей изображения из буфера кадра в теневой буфер, связанный с окном
void Application::CopyFrameBufferToSDLSurface()
{
	// Выполняется доступ к пикселям поверхности
	SDL_LockSurface(m_pMainSurface);
	const SDL_PixelFormat* pixelFormat = m_pMainSurface->format;

	// Проверка формата пикселей поверхности, для простоты только поддержка 32 битных.
	if (pixelFormat->BitsPerPixel == 32)
	{
		const Uint8 rShift = pixelFormat->Rshift;
		const Uint8 gShift = pixelFormat->Gshift;
		const Uint8 bShift = pixelFormat->Bshift;
		const Uint8 aShift = pixelFormat->Ashift;
		const Uint32 aMask = pixelFormat->Amask;

		const unsigned h = m_frameBuffer.GetHeight();
		const unsigned w = m_frameBuffer.GetWidth();

	    // Цикл с построчным копированием пикселей изображения. Адрес каждой последующей строки смещён относительно адреса предыдущей на pitch
		Uint8* pixels = reinterpret_cast<Uint8*>(m_pMainSurface->pixels);
		for (unsigned y = 0; y < h; ++y, pixels += m_pMainSurface->pitch)
		{
			// Вычисляются адреса начала строки буфера кадра и теневого буфера
			std::uint32_t const* srcLine = m_frameBuffer.GetPixels(y);
			Uint32* dstLine = reinterpret_cast<Uint32*>(pixels);

			// Когда формат пикселей буфера кадра и теневого буфера совпадают, то копируем обычной memcpy
			if (bShift == 0 && gShift == 8 && rShift == 16)
			{
				memcpy(dstLine, srcLine, w * sizeof(Uint32));
			}
			// В противном случае, каждый пиксель буфера кадра трансформируется в требуемый формат с манипулированием над битами на основе сдвигов.
			else
			{
				for (unsigned x = 0; x < w; ++x)
				{
					boost::uint32_t srcColor = srcLine[x];
					Uint32 dstColor = ((srcColor & 0xff) << bShift) | (((srcColor >> 8) & 0xff) << gShift) | (((srcColor >> 16) & 0xff) << rShift) | ((((srcColor >> 24)) << aShift) & aMask);
					dstLine[x] = dstColor;
				}
			}
		}
	}

	SDL_UnlockSurface(m_pMainSurface);
}

Uint32 SDLCALL Application::TimerCallback(Uint32 interval, void* param)
{
	/*
		Статический метод TimerCallback вызывается библиотекой SDL.

		Поскольку при инициализации таймера в качестве параметра таймера был передан указатель this экземпляра класса CApplication,
		здесь используется оператор приведения типа для обратного преобразования указателя void* к указателю на CApplication
		и происходит вызов метода OnTimer соответствующего экземпляра класса.
	*/
	Application* pMyApp = reinterpret_cast<Application*>(param);
	return pMyApp->OnTimer(interval);
}

Uint32 Application::OnTimer(Uint32 interval)
{
	unsigned renderedChunks = 0;
	unsigned totalChunks = 0;
	if (m_renderer.GetProgress(renderedChunks, totalChunks))
	{
		// Если формирование завершено, то заносим значение 0.
		interval = 0;
	}
	// Независимо от того, было ли завершено построение, необходимо принудительно пометить окно для последующего обновления.
	InvalidateMainSurface();

	// Интервал, через который должно произойти обновление.
	return interval;
}

void Application::InvalidateMainSurface()
{
	/*
	Считывается значение флага m_mainSurfaceUpdated. Значение данного флага, равное 1 сигнализирует о том, что основной поток приложения ранее выполнил обновление содержимого окна.
	*/
	bool redrawIsNeeded = m_mainSurfaceUpdated.load() == 1;

	/*
	* 
	Принудительное обновление содержимого окна приложения заключается в добавлении события SDL_VIDEOEXPOSE в очередь событий. 
	Т.к. доабвление данного события в очередь и его обработка выполняются разными потоками, 
	необходимо следить за тем, чтобы в очереди сообщений одновременно находилось не более одного8 события SDL_VIDEOEXPOSE.
	*/
	if (redrawIsNeeded)
	{
		// Выполняется сброс флага m_mainsurfaceUpdated, благодаря чему последующее добавление события SDL_VIDEOEXPOSE будет возможно только после обновления содержимого окна.
		m_mainSurfaceUpdated.store(0);

		// Событие SDL_VIDEOEXPOSE добавляется в очередь.
		SDL_Event evt;
		evt.type = SDL_VIDEOEXPOSE;
		SDL_PushEvent(&evt);
	}
}

void Application::AddSomePlane()
{
	/*
		Матрица трансформации плоскости
	*/
	CMatrix4d planeTransform;
	planeTransform.Translate(0, -2, -3);

	/*
		Материал плоскости
	*/
	ComplexMaterial planeMaterial;
	planeMaterial.SetDiffuseColor(CVector4f(0, 0, 1, 1));
	planeMaterial.SetSpecularColor(CVector4f(1, 1, 1, 1));
	planeMaterial.SetAmbientColor(CVector4f(0.0f, 0.2f, 0.2f, 1));

	AddPlane(CreatePhongShader(planeMaterial), 0, 1, 0, 0, planeTransform);
}

void Application::AddSomeLight()
{
	COmniLightPtr pLightFront(new COmniLightSource(CVector3d(0.f, 5.0, 10.f)));
	pLightFront->SetDiffuseIntensity(CVector4f(1, 1, 1, 1));
	pLightFront->SetSpecularIntensity(CVector4f(1, 1, 1, 1));
	pLightFront->SetAmbientIntensity(CVector4f(1, 1, 1, 1));
	m_scene.AddLightSource(pLightFront);
}

void Application::AddSomeCubes()
{
	// Матрица трансформации куба
	CMatrix4d cubeTransform;
	cubeTransform.Translate(-4, -0.5f, 0);
	cubeTransform.Scale(1, 1, 1);
	cubeTransform.Rotate(30, 0, 1, 0);
	cubeTransform.Rotate(-15, 1, 0, 0);

	//Материал куба
	ComplexMaterial cubeMaterial;
	cubeMaterial.SetDiffuseColor(CVector4f(1, 0, 0, 1));
	cubeMaterial.SetSpecularColor(CVector4f(1, 1, 1, 1));
	cubeMaterial.SetAmbientColor(CVector4f(0.2f, 0.2f, 0.2f, 1));
	cubeMaterial.SetSpecularCoefficient(2048);

	// Создаю PhongShader на основе материала, начальный размер = 1, центр и матрица трансформации
	AddCube(CreatePhongShader(cubeMaterial), 1, CVector3d(0, 0, 0), cubeTransform);
}

void Application::AddSomeTetrahedron()
{
	// Вершины
	CVector3d v0(-1, 0, 1);
	CVector3d v1(+1, 0, 1);
	CVector3d v2(0, 0, -1);
	CVector3d v3(0, 2, 0);
	std::vector<Vertex> vertices;
	vertices.push_back(Vertex(v0));
	vertices.push_back(Vertex(v1));
	vertices.push_back(Vertex(v2));
	vertices.push_back(Vertex(v3));

	// Грани
	std::vector<Face> faces;
	faces.push_back(Face(0, 2, 1));
	faces.push_back(Face(3, 0, 1));
	faces.push_back(Face(3, 1, 2));
	faces.push_back(Face(3, 2, 0));

	// Данные полигональной сетки
	CTriangleMeshData* pMeshData = CreateTriangleMeshData(vertices, faces);

	CMatrix4d transform;
	transform.Translate(3, 0.5f, -1);
	transform.Rotate(170, 0, 1, 0);
	CSimpleMaterial blue;
	blue.SetDiffuseColor(CVector4f(0.5f, 0.8f, 1, 1));

	AddTriangleMesh(CreateSimpleDiffuseShader(blue), pMeshData, transform);
}

void Application::AddSomeOctahedron()
{
	// Вершины
	CVector3d v0(0, 1, 0);
	CVector3d v1(1, 0, 0);
	CVector3d v2(0, -1, 0);
	CVector3d v3(-1, 0, 0);
	CVector3d v4(0, 0, 1);
	CVector3d v5(0, 0, -1);
	std::vector<Vertex> vertices;
	vertices.push_back(Vertex(v0));
	vertices.push_back(Vertex(v1));
	vertices.push_back(Vertex(v2));
	vertices.push_back(Vertex(v3));
	vertices.push_back(Vertex(v4));
	vertices.push_back(Vertex(v5));

	// Грани
	std::vector<Face> faces;
	faces.push_back(Face(2, 1, 4));
	faces.push_back(Face(1, 0, 4));
	faces.push_back(Face(0, 3, 4));
	faces.push_back(Face(3, 2, 4));
	faces.push_back(Face(2, 5, 1));
	faces.push_back(Face(1, 5, 0));
	faces.push_back(Face(0, 5, 3));
	faces.push_back(Face(3, 5, 2));

	// Данные полигональной сетки
	CTriangleMeshData* pMeshData = CreateTriangleMeshData(vertices, faces);

	CMatrix4d transform;
	transform.Translate(-3, 2, -5);
	transform.Scale(2, 2, 2);
	CSimpleMaterial violet;
	violet.SetDiffuseColor(CVector4f(0.8f, 0.0f, 0.8f, 1));

	AddTriangleMesh(CreateSimpleDiffuseShader(violet), pMeshData, transform);
}

void Application::AddSomeDodecahedron()
{
	CMatrix4d transform;
	transform.Translate(-2.5, -1, -3);
	transform.Rotate(75, 0, 1, 1);

	//Материал додекадра
	ComplexMaterial material;
	material.SetDiffuseColor(CVector4f(1, 0, 0, 1));
	material.SetSpecularColor(CVector4f(1, 1, 1, 1));
	material.SetAmbientColor(CVector4f(0.2f, 0.2f, 0.2f, 1));
	material.SetSpecularCoefficient(2048);

	AddDodecahedron(CreatePhongShader(material), transform);
}

void Application::AddSomeIcosahedron()
{
	CMatrix4d transform;
	transform.Translate(3, 0, 1);
	transform.Rotate(20, 0, 1, 1);

	//Материал икосаэдра
	ComplexMaterial material;
	material.SetDiffuseColor(CVector4f(0.5f, 0.2f, 0.9f, 1));
	material.SetSpecularColor(CVector4f(1, 1, 1, 1));
	material.SetAmbientColor(CVector4f(0.2f, 0.2f, 0.2f, 1));
	material.SetSpecularCoefficient(2048);

	AddIcosahedron(CreatePhongShader(material), transform);
}

void Application::AddSomeHyperbolicParaboloid()
{
	// TODO: зачем изначально мне вот эта матрица трансоформации
	CMatrix4d transform;
	transform.Rotate(-25, 0, 1, 0);
	transform.Translate(1, -1, 0);
	transform.Scale(0.7f, 0.7f, 0.7f);

	// Материал гиперболического параболоида
	ComplexMaterial material;
	material.SetDiffuseColor(CVector4f(1, 0.4f, 0.6f, 1));
	material.SetSpecularColor(CVector4f(1, 1, 1, 1));
	material.SetAmbientColor(CVector4f(0.2f, 0.2f, 0.2f, 1));
	material.SetSpecularCoefficient(256);

	AddHyperbolicParaboloid(CreatePhongShader(material), transform);
}


CSimpleDiffuseShader& Application::CreateSimpleDiffuseShader(CSimpleMaterial const& material)
{
	auto shader = std::make_unique<CSimpleDiffuseShader>(material);
	auto& shaderRef = *shader;
	m_shaders.emplace_back(std::move(shader));
	return shaderRef;
}

PhongShader& Application::CreatePhongShader(const ComplexMaterial& material)
{
	auto shader = std::make_unique<PhongShader>(material);
	auto& shaderRef = *shader;
	m_shaders.emplace_back(std::move(shader));
	return shaderRef;
}

CSceneObject& Application::AddPlane(IShader const& shader, double a, double b, double c, double d, CMatrix4d const& transform)
{
	const auto& plane = *m_geometryObjects.emplace_back(
		std::make_unique<CPlane>(a, b, c, d, transform));

	return AddSceneObject(plane, shader);
}

CSceneObject& Application::AddSceneObject(IGeometryObject const& object, IShader const& shader)
{
	auto obj = std::make_shared<CSceneObject>(object, shader);
	m_scene.AddObject(obj);

	return *obj;
}

CSceneObject& Application::AddCube(IShader const& shader, double size, CVector3d const& center, CMatrix4d const& transform)
{
	const auto& cube = *m_geometryObjects.emplace_back(
		std::make_unique<Cube>(size, center, transform));

	return AddSceneObject(cube, shader);
}

CSceneObject& Application::AddTriangleMesh(IShader const& shader, CTriangleMeshData const* pMeshData, CMatrix4d const& transform)
{
	const auto& mesh = *m_geometryObjects.emplace_back(std::make_unique<CTriangleMesh>(pMeshData, transform));
	return AddSceneObject(mesh, shader);
}

CTriangleMeshData* Application::CreateTriangleMeshData(std::vector<Vertex> const& vertices, std::vector<Face> const& faces)
{
	auto* meshData = m_triangleMeshDataObjects.emplace_back(std::make_unique<CTriangleMeshData>(vertices, faces)).get();
	return meshData;
}

CSceneObject& Application::AddDodecahedron(IShader const& shader, CMatrix4d const& transform)
{
	const auto& dodecahedron = *m_geometryObjects.emplace_back(
		std::make_unique<Dodecahedron>(transform));

	return AddSceneObject(dodecahedron, shader);
}

CSceneObject& Application::AddIcosahedron(IShader const& shader, CMatrix4d const& transform)
{
	const auto& icosahedron = *m_geometryObjects.emplace_back(
		std::make_unique<Icosahedron>(transform));

	return AddSceneObject(icosahedron, shader);
}

CSceneObject& Application::AddHyperbolicParaboloid(IShader const& shader, CMatrix4d const& transform)
{
	const auto& hyperbolicParaboloid = *m_geometryObjects.emplace_back(
		std::make_unique<HyperbolicParaboloid>(transform));

	return AddSceneObject(hyperbolicParaboloid, shader);
}

