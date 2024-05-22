#include "HyperbolicParaboloid.h"
#include "../../Intersection/Intersection.h"
#include "../../Ray/Ray.h"

HyperbolicParaboloid::HyperbolicParaboloid(CMatrix4d const& transform)
	: CGeometryObjectImpl(transform)
{
}

bool HyperbolicParaboloid::Hit(CRay const& ray, CIntersection& intersection) const
{
	// Вычисляем обратно преобразованный луч (вместо вполнения прямого преобразования объекта)
	CRay invRay = Transform(ray, GetInverseTransform());

	unsigned numHits = 0; // Количество точек пересечения
	double hitTimes[2];

	/*
		Начало и направление луча
	*/
	CVector3d const& start = invRay.GetStart();
	CVector3d const& dir = invRay.GetDirection();

	/*
		Коэффициенты квадратного уравнения боковой стенки конического цилиндра
	*/
	double a = Sqr(dir.x) - Sqr(dir.z);
	double b = 2 * (start.x * dir.x) - 2 * (start.z * dir.z) - dir.y;
	double c = Sqr(start.x) - Sqr(start.z) - start.y;

	// Дискриминант квадратного уравнения
	double discr = b * b - 4 * a * c;

	// Время, которое луч проходит из точки испускания, не испытывая столкновения
	// Нужно для того, чтобы отраженный/преломленный луч мог оторваться от границы объекта после столкновения
	static const double HIT_TIME_EPSILON = 1e-6;

	/*
		Если дискриминант неотрицательный, то есть точки пересечения луча поверхностью
	*/
	if (discr > 0)
	{
		double invDoubleA = 1.0 / (2 * a);
		double discRoot = sqrt(discr);

		// Первый корень квадратного уравнения - время столкновения с боковой стенкой
		double t = (-b - discRoot) * invDoubleA;

		// Нас не интересуют пересечения, происходящие "в прошлом" луча
		if (t > HIT_TIME_EPSILON)
		{
			// Проверяем координату z точки пересечения. Она не должна выходить за пределы
			// диапазона 0..1
			double hitX = start.x + dir.x * t;
			double hitY = start.z + dir.z * t;
			if (hitX >= -1 && hitX <= 1 &&
				hitY >= -1 && hitY <= 1)
			{
				hitTimes[numHits++] = t;
			}
		}

		// Второй корень квадратного уравнения - время столкновения с боковой стенкой
		t = (-b + discRoot) * invDoubleA;
		// Нас не интересуют пересечения, происходящие "в прошлом" луча
		if (t > HIT_TIME_EPSILON)
		{
			// Проверяем координату z точки пересечения. Она не должна выходить за пределы
			// диапазона 0..1
			double hitX = start.x + dir.x * t;
			double hitY = start.z + dir.z * t;
			if (hitX >= -1 && hitX <= 1 &&
				hitY >= -1 && hitY <= 1)
			{
				hitTimes[numHits++] = t;
			}
		}
	}

	// Длина проекции луча на ось z, меньше которой считается, что пересечений с основанием и крышкой нет
	static const double EPSILON = 1e-8;

	// Если ни одного пересечения не было найдено, дальнейшие вычисления проводить бессмысленно
	if (numHits == 0)
	{
		return false;
	}

	/*
		Упорядочиваем события столкновения в порядке возрастания времени столкновения
	*/
	if (numHits == 2)
	{
		if (hitTimes[0] > hitTimes[1])
		{
			std::swap(hitTimes[0], hitTimes[1]);
		}
	}

	// Для всех найденных точек пересечения собираем полную информацию и
	// добавляем ее в объект intersection
	for (unsigned i = 0; i < numHits; ++i)
	{
		double const& hitTime = hitTimes[i];

		/*
			Вычисляем координаты точки пересечения
		*/
		CVector3d hitPoint = ray.GetPointAtTime(hitTime);
		CVector3d hitPointInObjectSpace = invRay.GetPointAtTime(hitTime);
		CVector3d hitNormalInObjectSpace;

		// TODO: z -> y
		hitNormalInObjectSpace = CVector3d(2 * hitPointInObjectSpace.x, -2 * hitPointInObjectSpace.z, -1);

		// Скалярное произведение Dot(a,b) = ax * bx + ay*by + az * bz;
		auto nDotR = Dot(hitNormalInObjectSpace, dir);
		// Если положительна, то в сторону источника света, иначе обратно
		if (nDotR > 0)
		{
			hitNormalInObjectSpace = -hitNormalInObjectSpace;
		}

		/*
		*	Собираем информацию о точке столкновения
		*/
		CVector3d hitNormal = GetNormalMatrix() * hitNormalInObjectSpace;


		CHitInfo hit(
			hitTime, *this,
			hitPoint, hitPointInObjectSpace,
			hitNormal, hitNormalInObjectSpace);

		// Добавляем точку столкновения в список
		intersection.AddHit(hit);
	}

	// Возвращаем true, если было найдено хотя бы одно пересечение
	return intersection.GetHitsCount() > 0;
}