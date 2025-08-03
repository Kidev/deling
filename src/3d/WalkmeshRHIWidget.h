/****************************************************************************
 ** Deling Final Fantasy VIII Field Editor
 ** Copyright (C) 2009-2024 Arzel Jérôme <myst6re@gmail.com>
 **
 ** This program is free software: you can redistribute it and/or modify
 ** it under the terms of the GNU General Public License as published by
 ** the Free Software Foundation, either version 3 of the License, or
 ** (at your option) any later version.
 **
 ** This program is distributed in the hope that it will be useful,
 ** but WITHOUT ANY WARRANTY; without even the implied warranty of
 ** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 ** GNU General Public License for more details.
 **
 ** You should have received a copy of the GNU General Public License
 ** along with this program.  If not, see <http://www.gnu.org/licenses/>.
 ****************************************************************************/
#pragma once

#include <QtWidgets>
#include <QRhiWidget>
#include "Field.h"
#include "Renderer.h"

class WalkmeshRHIWidget : public QRhiWidget
{
	Q_OBJECT
public:
	explicit WalkmeshRHIWidget(QWidget *parent = nullptr);
	virtual ~WalkmeshRHIWidget() override;
	void clear();
	void fill(Field *data);
	void updatePerspective();

public slots:
	void setXRotation(int);
	void setYRotation(int);
	void setZRotation(int);
	void setZoom(int);
	void resetCamera();
	void setCurrentFieldCamera(int camID);
	void setBackgroundVisible(bool show);
	void setSelectedTriangle(int triangle);
	void setSelectedDoor(int door);
	void setSelectedGate(int gate);
	void setLineToDraw(const Vertex vertex[2]);
	void clearLineToDraw();

protected:
	virtual void initialize(QRhiCommandBuffer *cb) override;
	virtual void render(QRhiCommandBuffer *cb) override;
	virtual void releaseResources() override;
	virtual void timerEvent(QTimerEvent *event) override;
	virtual void wheelEvent(QWheelEvent *event) override;
	virtual void mousePressEvent(QMouseEvent *event) override;
	virtual void mouseMoveEvent(QMouseEvent *event) override;
	virtual void keyPressEvent(QKeyEvent *event) override;
	virtual void focusInEvent(QFocusEvent *event) override;
	virtual void focusOutEvent(QFocusEvent *event) override;

private:
	void computeFov();
	void drawBackground(QRhiCommandBuffer *cb);
	double distance;
	float xRot, yRot, zRot;
	float xTrans, yTrans, transStep;
	int lastKeyPressed;
	int camID;
	int _selectedTriangle;
	int _selectedDoor;
	int _selectedGate;
	Vertex _lineToDrawPoint1, _lineToDrawPoint2;
	double fovy;
	Field *data;
	QPoint moveStart;
	int curFrame;
	Renderer *rhiRenderer;
	QMatrix4x4 mProjection;
	QRhiTexture *backgroundTexture;
	bool _drawLine;
	bool _backgroundVisible;
	bool _initialized;
};