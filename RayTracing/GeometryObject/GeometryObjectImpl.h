﻿#pragma once
#include "IGeometryObject.h"
#include "../Matrix/Matrix4.h"

/*
Реализация функционала геометрических объектов, не зависящего от типа объекта
*/
class CGeometryObjectImpl : public IGeometryObject
{
public:
	/*
	Конструктор, принимающий матрицу, задающую трансформацию объекта
	*/
	CGeometryObjectImpl(CMatrix4d const& transform = CMatrix4d())
	{
		SetTransform(transform);
	}

	/*
	Возвращает матрицу трансформации объекта
	*/
	CMatrix4d const& GetTransform() const override
	{
		return m_transform;
	}

	/*
		Задает матрицу трансформации объекта
	*/
	void SetTransform(CMatrix4d const& transform) override
	{
		m_transform = transform;

		// Также вычисляем матрицу обратного преобразования
		m_invTransform = transform.GetInverseMatrix();
		
		// и матрицу нормали
		m_normalMatrix.SetRow(0, m_invTransform.GetColumn(0));
		m_normalMatrix.SetRow(1, m_invTransform.GetColumn(1));
		m_normalMatrix.SetRow(2, m_invTransform.GetColumn(2));
		
		// Уведомляем наследников об изменении матрицы преобразования
		OnUpdateTransform();
	}

	/*
		Возвращает матрицу, обратную матрице трансформации.
		Как правило, гораздо проще применить обратную трансформацию к лучу и найти
		точку его пересечения с базовой формой геометрического объекта, нежели выполнять
		прямую трансформацию объекта и искать пересечени луча с ним
	*/
	CMatrix4d const& GetInverseTransform() const
	{
		return m_invTransform;
	}

	// Матрица нормали
	CMatrix3d const& GetNormalMatrix() const
	{
		return m_normalMatrix;
	}
protected:
	/*
		Вызывается всякий раз, когда у объекта изменяется матрица трансформации
		Может быть перегружен в классах-наследниках для выполнения связанных с этим операций
	*/
	virtual void OnUpdateTransform()
	{
		// Здесь мы ничего не делаем, но классы-наследники будут перегружать данный метод
	}
private:
	// Матрица трансформации объекта
	CMatrix4d m_transform;

	// Матрица нормалей
	CMatrix3d m_normalMatrix;

	// Обратная матрица
	CMatrix4d m_invTransform;
};
