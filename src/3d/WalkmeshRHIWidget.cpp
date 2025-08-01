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
#include "WalkmeshRHIWidget.h"

WalkmeshRHIWidget::WalkmeshRHIWidget(QWidget *parent)
    : QRhiWidget(parent), distance(0.0), xRot(0.0f), yRot(0.0f), zRot(0.0f),
      xTrans(0.0f), yTrans(0.0f), transStep(360.0f), lastKeyPressed(-1),
      camID(0), _selectedTriangle(-1), _selectedDoor(-1), _selectedGate(-1),
      _lineToDrawPoint1(Vertex()), _lineToDrawPoint2(Vertex()), fovy(70.0),
      data(nullptr), curFrame(0), rhiRenderer(nullptr),
      backgroundTexture(nullptr), _drawLine(false), _backgroundVisible(true),
      _initialized(false)
{
	// setMouseTracking(true);
	// startTimer(100);
}

WalkmeshRHIWidget::~WalkmeshRHIWidget()
{
	releaseResources();
}

void WalkmeshRHIWidget::releaseResources()
{
	if (rhiRenderer != nullptr) {
		delete rhiRenderer;
		rhiRenderer = nullptr;
	}

	if (backgroundTexture) {
		delete backgroundTexture;
		backgroundTexture = nullptr;
	}

	_initialized = false;
}

void WalkmeshRHIWidget::timerEvent(QTimerEvent *)
{
	update();
}

void WalkmeshRHIWidget::clear()
{
	data = nullptr;

	if (backgroundTexture) {
		delete backgroundTexture;
		backgroundTexture = nullptr;
	}

	update();

	if (rhiRenderer) {
		rhiRenderer->reset();
	}
}

void WalkmeshRHIWidget::fill(Field *data)
{
	this->data = data;

	if (_initialized && data && data->getBackgroundFile()) {
		QImage bgImage = data->getBackgroundFile()->background();
		if (!bgImage.isNull()) {
			if (backgroundTexture) {
				delete backgroundTexture;
			}
			backgroundTexture =
			    rhi()->newTexture(QRhiTexture::RGBA8, bgImage.size());
			backgroundTexture->create();

			QRhiResourceUpdateBatch *batch = rhi()->nextResourceUpdateBatch();
			batch->uploadTexture(backgroundTexture, bgImage);
			rhiRenderer->commitResourceUpdates(batch);
		}
	}

	updatePerspective();
	resetCamera();
}

void WalkmeshRHIWidget::computeFov()
{
	if (data && data->hasCaFile() && data->getCaFile()->cameraCount() > 0
	    && camID < data->getCaFile()->cameraCount()) {
		const Camera &cam = data->getCaFile()->camera(camID);
		fovy = (2 * atan(240.0 / (2.0 * cam.camera_zoom))) * 57.29577951;
	} else {
		fovy = 70.0;
	}
}

void WalkmeshRHIWidget::updatePerspective()
{
	computeFov();
	update();
}

void WalkmeshRHIWidget::initialize(QRhiCommandBuffer *cb)
{
	Q_UNUSED(cb);

	if (rhiRenderer == nullptr) {
		rhiRenderer = new Renderer(this, rhi());
		rhiRenderer->initialize(renderTarget()->renderPassDescriptor());
		_initialized = true;

		// Initialize background texture if we have data
		if (data && data->getBackgroundFile()) {
			QImage bgImage = data->getBackgroundFile()->background();
			if (!bgImage.isNull()) {
				backgroundTexture =
				    rhi()->newTexture(QRhiTexture::RGBA8, bgImage.size());
				backgroundTexture->create();

				QRhiResourceUpdateBatch *batch =
				    rhi()->nextResourceUpdateBatch();
				batch->uploadTexture(backgroundTexture, bgImage);
				rhiRenderer->commitResourceUpdates(batch);
			}
		}
	}
}

void WalkmeshRHIWidget::render(QRhiCommandBuffer *cb)
{
	if (!data || !_initialized) {
		return;
	}

	if (rhiRenderer->hasError()) {
		return;
	}

	if (_backgroundVisible) {
		drawBackground(cb);
	}

	const QSize pixelSize = renderTarget()->pixelSize();
	mProjection.setToIdentity();
	mProjection.perspective(
	    fovy, (float)pixelSize.width() / (float)pixelSize.height(), 0.001f,
	    1000.0f);
	rhiRenderer->bindProjectionMatrix(mProjection);

	QMatrix4x4 mModel;
	mModel.translate(xTrans, yTrans, distance);
	mModel.rotate(xRot, 1.0f, 0.0f, 0.0f);
	mModel.rotate(yRot, 0.0f, 1.0f, 0.0f);
	mModel.rotate(zRot, 0.0f, 0.0f, 1.0f);

	QMatrix4x4 mView;

	if (data->hasCaFile() && data->getCaFile()->cameraCount() > 0
	    && camID < data->getCaFile()->cameraCount()) {
		const Camera &cam = data->getCaFile()->camera(camID);

		double camAxisXx = cam.camera_axis[0].x / 4096.0;
		double camAxisXy = cam.camera_axis[0].y / 4096.0;
		double camAxisXz = cam.camera_axis[0].z / 4096.0;

		double camAxisYx = -cam.camera_axis[1].x / 4096.0;
		double camAxisYy = -cam.camera_axis[1].y / 4096.0;
		double camAxisYz = -cam.camera_axis[1].z / 4096.0;

		double camAxisZx = cam.camera_axis[2].x / 4096.0;
		double camAxisZy = cam.camera_axis[2].y / 4096.0;
		double camAxisZz = cam.camera_axis[2].z / 4096.0;

		double camPosX = cam.camera_position[0] / 4096.0;
		double camPosY = -cam.camera_position[1] / 4096.0;
		double camPosZ = cam.camera_position[2] / 4096.0;

		double tx =
		    -(camPosX * camAxisXx + camPosY * camAxisYx + camPosZ * camAxisZx);
		double ty =
		    -(camPosX * camAxisXy + camPosY * camAxisYy + camPosZ * camAxisZy);
		double tz =
		    -(camPosX * camAxisXz + camPosY * camAxisYz + camPosZ * camAxisZz);

		const QVector3D eye(tx, ty, tz),
		    center(tx + camAxisZx, ty + camAxisZy, tz + camAxisZz),
		    up(camAxisYx, camAxisYy, camAxisYz);
		mView.lookAt(eye, center, up);
	}

	rhiRenderer->bindModelMatrix(mModel);
	rhiRenderer->bindViewMatrix(mView);

	if (data->hasIdFile()) {
		rhiRenderer->clearVertices();

		int i = 0;

		for (const Triangle &triangle : data->getIdFile()->getTriangles()) {
			const Access &access = data->getIdFile()->access(i);

			// Vertex info
			QVector3D positionA(triangle.vertices[0].x / 4096.0,
			                    triangle.vertices[0].y / 4096.0,
			                    triangle.vertices[0].z / 4096.0),
			    positionB(triangle.vertices[1].x / 4096.0,
			              triangle.vertices[1].y / 4096.0,
			              triangle.vertices[1].z / 4096.0),
			    positionC(triangle.vertices[2].x / 4096.0,
			              triangle.vertices[2].y / 4096.0,
			              triangle.vertices[2].z / 4096.0);
			QRgba64 color1 = QRgba64::fromArgb32(
			            (i == _selectedTriangle
			                 ? 0xFFFF9000
			                 : (access.a[0] == -1 ? 0xFF6699CC : 0xFFFFFFFF))),
			        color2 = QRgba64::fromArgb32(
			            (i == _selectedTriangle
			                 ? 0xFFFF9000
			                 : (access.a[1] == -1 ? 0xFF6699CC : 0xFFFFFFFF))),
			        color3 = QRgba64::fromArgb32(
			            (i == _selectedTriangle
			                 ? 0xFFFF9000
			                 : (access.a[2] == -1 ? 0xFF6699CC : 0xFFFFFFFF)));
			QVector2D texcoord;

			// Line
			rhiRenderer->bufferVertex(positionA, color1, texcoord);
			rhiRenderer->bufferVertex(positionB, color1, texcoord);

			// Line
			rhiRenderer->bufferVertex(positionB, color2, texcoord);
			rhiRenderer->bufferVertex(positionC, color2, texcoord);

			// Line
			rhiRenderer->bufferVertex(positionC, color3, texcoord);
			rhiRenderer->bufferVertex(positionA, color3, texcoord);

			++i;
		}

		if (!_drawLine && data->hasInfFile()) {
			InfFile *inf = data->getInfFile();

			for (const Gateway &gate : inf->getGateways()) {
				if (gate.fieldId != 0x7FFF) {
					// Vertex info
					QVector3D positionA(gate.exitLine[0].x / 4096.0,
					                    gate.exitLine[0].y / 4096.0,
					                    gate.exitLine[0].z / 4096.0),
					    positionB(gate.exitLine[1].x / 4096.0,
					              gate.exitLine[1].y / 4096.0,
					              gate.exitLine[1].z / 4096.0);
					QRgba64 color = QRgba64::fromArgb32(0xFFFF0000);
					QVector2D texcoord;

					rhiRenderer->bufferVertex(positionA, color, texcoord);
					rhiRenderer->bufferVertex(positionB, color, texcoord);
				}
			}

			for (const Trigger &trigger : inf->getTriggers()) {
				if (trigger.doorID != 0xFF) {
					// Vertex info
					QVector3D positionA(trigger.trigger_line[0].x / 4096.0,
					                    trigger.trigger_line[0].y / 4096.0,
					                    trigger.trigger_line[0].z / 4096.0),
					    positionB(trigger.trigger_line[1].x / 4096.0,
					              trigger.trigger_line[1].y / 4096.0,
					              trigger.trigger_line[1].z / 4096.0);
					QRgba64 color = QRgba64::fromArgb32(0xFF00FF00);
					QVector2D texcoord;

					rhiRenderer->bufferVertex(positionA, color, texcoord);
					rhiRenderer->bufferVertex(positionB, color, texcoord);
				}
			}
		}

		if (_drawLine) {
			// Vertex info
			QVector3D positionA(_lineToDrawPoint1.x / 4096.0,
			                    _lineToDrawPoint1.y / 4096.0,
			                    _lineToDrawPoint1.z / 4096.0),
			    positionB(_lineToDrawPoint2.x / 4096.0,
			              _lineToDrawPoint2.y / 4096.0,
			              _lineToDrawPoint2.z / 4096.0);
			QRgba64 color = QRgba64::fromArgb32(0xFFFF00FF);
			QVector2D texcoord;

			rhiRenderer->bufferVertex(positionA, color, texcoord);
			rhiRenderer->bufferVertex(positionB, color, texcoord);
		}

		rhiRenderer->draw(cb, RendererPrimitiveType::PT_LINES);

		if (_selectedTriangle >= 0
		    && _selectedTriangle < data->getIdFile()->triangleCount()) {
			const Triangle &triangle =
			    data->getIdFile()->triangle(_selectedTriangle);

			rhiRenderer->clearVertices();

			// Vertex info
			QVector3D positionA(triangle.vertices[0].x / 4096.0,
			                    triangle.vertices[0].y / 4096.0,
			                    triangle.vertices[0].z / 4096.0),
			    positionB(triangle.vertices[1].x / 4096.0,
			              triangle.vertices[1].y / 4096.0,
			              triangle.vertices[1].z / 4096.0),
			    positionC(triangle.vertices[2].x / 4096.0,
			              triangle.vertices[2].y / 4096.0,
			              triangle.vertices[2].z / 4096.0);
			QRgba64 color = QRgba64::fromArgb32(0xFFFF9000);
			QVector2D texcoord;

			// Triangle
			rhiRenderer->bufferVertex(positionA, color, texcoord);
			rhiRenderer->bufferVertex(positionB, color, texcoord);
			rhiRenderer->bufferVertex(positionC, color, texcoord);
		}

		if (data->hasInfFile()) {
			if (_selectedGate >= 0 && _selectedGate < 12) {
				const Gateway &gate =
				    data->getInfFile()->getGateway(_selectedGate);
				if (gate.fieldId != 0x7FFF) {
					// Vertex info
					QVector3D positionA(gate.exitLine[0].x / 4096.0,
					                    gate.exitLine[0].y / 4096.0,
					                    gate.exitLine[0].z / 4096.0),
					    positionB(gate.exitLine[1].x / 4096.0,
					              gate.exitLine[1].y / 4096.0,
					              gate.exitLine[1].z / 4096.0);
					QRgba64 color = QRgba64::fromArgb32(0xFFFF0000);
					QVector2D texcoord;

					rhiRenderer->bufferVertex(positionA, color, texcoord);
					rhiRenderer->bufferVertex(positionB, color, texcoord);
				}
			}

			if (_selectedDoor >= 0 && _selectedDoor < 12) {
				const Trigger &trigger =
				    data->getInfFile()->getTrigger(_selectedDoor);
				if (trigger.doorID != 0xFF) {
					// Vertex info
					QVector3D positionA(trigger.trigger_line[0].x / 4096.0,
					                    trigger.trigger_line[0].y / 4096.0,
					                    trigger.trigger_line[0].z / 4096.0),
					    positionB(trigger.trigger_line[1].x / 4096.0,
					              trigger.trigger_line[1].y / 4096.0,
					              trigger.trigger_line[1].z / 4096.0);
					QRgba64 color = QRgba64::fromArgb32(0xFF00FF00);
					QVector2D texcoord;

					rhiRenderer->bufferVertex(positionA, color, texcoord);
					rhiRenderer->bufferVertex(positionB, color, texcoord);
				}
			}
		}

		rhiRenderer->draw(cb, RendererPrimitiveType::PT_POINTS, 7.0f);
	}
}

void WalkmeshRHIWidget::drawBackground(QRhiCommandBuffer *cb)
{
	if (data->getBackgroundFile() && backgroundTexture) {
		QMatrix4x4 mBG;

		rhiRenderer->bindProjectionMatrix(mBG);
		rhiRenderer->bindViewMatrix(mBG);
		rhiRenderer->bindModelMatrix(mBG);

		rhiRenderer->clearVertices();

		// Create fullscreen quad vertices
		QVector3D positions[] = { QVector3D(-1.0f, -1.0f, 1.0f),
			                      QVector3D(-1.0f, 1.0f, 1.0f),
			                      QVector3D(1.0f, -1.0f, 1.0f),
			                      QVector3D(1.0f, 1.0f, 1.0f) };

		QVector2D texcoords[] = { QVector2D(0.0f, 1.0f), QVector2D(0.0f, 0.0f),
			                      QVector2D(1.0f, 1.0f),
			                      QVector2D(1.0f, 0.0f) };

		QRgba64 white = QRgba64::fromRgba(0xFF, 0xFF, 0xFF, 0xFF);

		// Triangle 1
		rhiRenderer->bufferVertex(positions[0], white, texcoords[0]);
		rhiRenderer->bufferVertex(positions[1], white, texcoords[1]);
		rhiRenderer->bufferVertex(positions[2], white, texcoords[2]);

		// Triangle 2
		rhiRenderer->bufferVertex(positions[1], white, texcoords[1]);
		rhiRenderer->bufferVertex(positions[3], white, texcoords[3]);
		rhiRenderer->bufferVertex(positions[2], white, texcoords[2]);

		rhiRenderer->bindTexture(backgroundTexture);
		rhiRenderer->draw(cb, RendererPrimitiveType::PT_TRIANGLES);
	}
}

void WalkmeshRHIWidget::wheelEvent(QWheelEvent *event)
{
	setFocus();
	distance += event->pixelDelta().x() / 4096.0;
	update();
}

void WalkmeshRHIWidget::mousePressEvent(QMouseEvent *event)
{
	setFocus();
	if (event->button() == Qt::MiddleButton) {
		distance = -35;
		update();
	} else if (event->button() == Qt::LeftButton) {
		moveStart = event->pos();
	}
}

void WalkmeshRHIWidget::mouseMoveEvent(QMouseEvent *event)
{
	if (event->button() == Qt::LeftButton) {
		xTrans += (event->pos().x() - moveStart.x()) / 4096.0;
		yTrans -= (event->pos().y() - moveStart.y()) / 4096.0;
		moveStart = event->pos();
		update();
	}
}

void WalkmeshRHIWidget::keyPressEvent(QKeyEvent *event)
{
	if (lastKeyPressed == event->key()
	    && (event->key() == Qt::Key_Left || event->key() == Qt::Key_Right
	        || event->key() == Qt::Key_Down || event->key() == Qt::Key_Up)) {
		if (transStep > 100.0f) {
			transStep *= 0.90f; // accelerator
		}
	} else {
		transStep = 360.0f;
	}
	lastKeyPressed = event->key();

	switch (event->key()) {
	case Qt::Key_Left:
		xTrans += 1.0f / transStep;
		update();
		break;
	case Qt::Key_Right:
		xTrans -= 1.0f / transStep;
		update();
		break;
	case Qt::Key_Down:
		yTrans += 1.0f / transStep;
		update();
		break;
	case Qt::Key_Up:
		yTrans -= 1.0f / transStep;
		update();
		break;
	default:
		QWidget::keyPressEvent(event);
		return;
	}
}

void WalkmeshRHIWidget::focusInEvent(QFocusEvent *event)
{
	grabKeyboard();
	QWidget::focusInEvent(event);
}

void WalkmeshRHIWidget::focusOutEvent(QFocusEvent *event)
{
	releaseKeyboard();
	QWidget::focusOutEvent(event);
}

static void qNormalizeAngle(int &angle)
{
	while (angle < 0)
		angle += 360 * 16;
	while (angle > 360 * 16)
		angle -= 360 * 16;
}

void WalkmeshRHIWidget::setXRotation(int angle)
{
	qNormalizeAngle(angle);
	if (angle != xRot) {
		xRot = angle;
		update();
	}
}

void WalkmeshRHIWidget::setYRotation(int angle)
{
	qNormalizeAngle(angle);
	if (angle != yRot) {
		yRot = angle;
		update();
	}
}

void WalkmeshRHIWidget::setZRotation(int angle)
{
	qNormalizeAngle(angle);
	if (angle != zRot) {
		zRot = angle;
		update();
	}
}

void WalkmeshRHIWidget::setZoom(int zoom)
{
	distance = zoom / 4096.0;
}

void WalkmeshRHIWidget::resetCamera()
{
	distance = 0;
	zRot = yRot = xRot = 0;
	xTrans = yTrans = 0;
	update();
}

void WalkmeshRHIWidget::setCurrentFieldCamera(int camID)
{
	this->camID = camID;
	updatePerspective();
}

void WalkmeshRHIWidget::setSelectedTriangle(int triangle)
{
	_selectedTriangle = triangle;
	update();
}

void WalkmeshRHIWidget::setSelectedDoor(int door)
{
	_selectedDoor = door;
	update();
}

void WalkmeshRHIWidget::setSelectedGate(int gate)
{
	_selectedGate = gate;
	update();
}

void WalkmeshRHIWidget::setLineToDraw(const Vertex vertex[2])
{
	_lineToDrawPoint1 = vertex[0];
	_lineToDrawPoint2 = vertex[1];
	_drawLine = true;
	update();
}

void WalkmeshRHIWidget::clearLineToDraw()
{
	_drawLine = false;
	update();
}

void WalkmeshRHIWidget::setBackgroundVisible(bool show)
{
	if (_backgroundVisible != show) {
		_backgroundVisible = show;
		update();
	}
}
