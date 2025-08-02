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
#include "WorldmapRHIWidget.h"

WorldmapRHIWidget::WorldmapRHIWidget(QWidget *parent, Qt::WindowFlags f)
    : QRhiWidget(parent, f), _map(nullptr), _distance(-0.714248f),
      _xRot(-90.0f), _yRot(180.0f), _zRot(180.0f), _xTrans(-0.5f),
      _yTrans(0.5f), _transStep(360.0f), _lastKeyPressed(-1), _texture(-1),
      _segmentGroupId(-1), _segmentId(-1), _blockId(-1), _groundType(-1),
      _polyId(-1), _clutId(-1), _limits(QRect(0, 0, 32, 24)),
      _megaTexture(nullptr), rhiRenderer(nullptr),
      _segmentFiltering(Map::NoFiltering), _initialized(false)
{
	setMouseTracking(true);
}

WorldmapRHIWidget::~WorldmapRHIWidget()
{
	releaseResources();
}

void WorldmapRHIWidget::releaseResources()
{
	if (rhiRenderer) {
		delete rhiRenderer;
		rhiRenderer = nullptr;
	}

	if (_megaTexture) {
		delete _megaTexture;
		_megaTexture = nullptr;
	}

	_initialized = false;
}

void WorldmapRHIWidget::resetCamera()
{
	_xRot = -90.0f;
	_yRot = 180.0f;
	_zRot = 180.0f;
	update();
}

void WorldmapRHIWidget::setMap(Map *map)
{
	_map = map;
	if (_initialized) {
		importVertices();
	}
	update();
}

void WorldmapRHIWidget::setLimits(const QRect &rect)
{
	_limits = rect;
	if (_initialized) {
		importVertices();
	}
	update();
}

void WorldmapRHIWidget::setXTrans(float trans)
{
	_xTrans = trans;
	update();
}

void WorldmapRHIWidget::setYTrans(float trans)
{
	_yTrans = trans;
	update();
}

void WorldmapRHIWidget::setZTrans(float trans)
{
	_distance = trans;
	update();
}

void WorldmapRHIWidget::setXRot(float rot)
{
	_xRot = rot;
	update();
}

void WorldmapRHIWidget::setYRot(float rot)
{
	_yRot = rot;
	update();
}

void WorldmapRHIWidget::setZRot(float rot)
{
	_zRot = rot;
	update();
}

void WorldmapRHIWidget::setTexture(int texture)
{
	_texture = texture;
	update();
}

void WorldmapRHIWidget::setSegmentGroupId(int segmentGroupId)
{
	_segmentGroupId = segmentGroupId;
	update();
}

void WorldmapRHIWidget::setSegmentId(int segmentId)
{
	_segmentId = segmentId;
	update();
}

void WorldmapRHIWidget::setBlockId(int blockId)
{
	_blockId = blockId;
	update();
}

void WorldmapRHIWidget::setGroundType(int groundType)
{
	_groundType = groundType;
	update();
}

void WorldmapRHIWidget::setPolyId(int polyId)
{
	_polyId = polyId;
	update();
}

void WorldmapRHIWidget::setClutId(int clutId)
{
	_clutId = clutId;
	update();
}

void WorldmapRHIWidget::setSegmentFiltering(Map::SegmentFiltering filtering)
{
	_segmentFiltering = filtering;

	if (_initialized) {
		importVertices();
	}
	update();
}

void WorldmapRHIWidget::dumpCurrent()
{
	if (!_map || _segmentId < 0 || _blockId < 0 || _polyId < 0)
		return;

	const MapPoly &poly = _map->segments()
	                          .at(_segmentId)
	                          .blocks()
	                          .at(_blockId)
	                          .polygons()
	                          .at(_polyId);
	qDebug() << QString::number(poly.flags1(), 16)
	         << QString::number(poly.flags2(), 16) << poly.groundType()
	         << "texPage" << poly.texPage() << "clutId" << poly.clutId()
	         << "hasTexture" << poly.hasTexture() << "isMonochrome"
	         << poly.isMonochrome();
	for (const TexCoord &coord : poly.texCoords()) {
		qDebug() << "texcoord" << coord.x << coord.y;
	}
	for (const Vertex &vertex : poly.vertices()) {
		qDebug() << "vertex" << vertex.x << vertex.y << vertex.z;
	}
}

void WorldmapRHIWidget::initialize(QRhiCommandBuffer *cb)
{
	Q_UNUSED(cb);

	if (rhiRenderer == nullptr) {
		rhiRenderer = new Renderer(this, rhi());
		rhiRenderer->initialize(renderTarget()->renderPassDescriptor());
		_initialized = true;
		importVertices();
	}
}

static quint16 normalizeY(qint16 y)
{
	return quint16(-(y - 128));
}

void WorldmapRHIWidget::importVertices()
{
	if (nullptr == _map || rhiRenderer == nullptr || !_initialized) {
		return;
	}

	QImage megaImage = _map->megaImage();
	if (_megaTexture) {
		delete _megaTexture;
	}
	_megaTexture = rhi()->newTexture(QRhiTexture::RGBA8, megaImage.size());
	_megaTexture->create();

	QRhiResourceUpdateBatch *batch = rhi()->nextResourceUpdateBatch();
	batch->uploadTexture(_megaTexture, megaImage);

	const int segmentPerLine = 32, blocksPerLine = 4,
	          diffSize = _limits.width() - _limits.height();
	const float scaleVect = 2048.0f,
	            scaleTexX = float(_megaTexture->pixelSize().width() - 1),
	            scaleTexY = float(_megaTexture->pixelSize().height() - 1),
	            scale = _limits.width() * blocksPerLine;
	const float xShift =
	    -_limits.x() * blocksPerLine
	    + (diffSize < 0 ? -diffSize : 0) * blocksPerLine / 2.0f;
	const float zShift = -_limits.y() * blocksPerLine
	                     + (diffSize > 0 ? diffSize : 0) * blocksPerLine / 2.0f;
	int xs = 0, ys = 0;

	QList<MapSegment> segments = _map->segments(_segmentFiltering);
	QRgba64 color = QRgba64::fromRgba(0xFF, 0xFF, 0xFF, 0xFF);

	rhiRenderer->clearVertices();

	foreach (const MapSegment &segment, segments) {
		int xb = 0, yb = 0;
		foreach (const MapBlock &block, segment.blocks()) {
			foreach (const MapPoly &poly, block.polygons()) {
				const int x = xs * blocksPerLine + xb,
				          z = ys * blocksPerLine + yb;

				if (poly.vertices().size() != 3) {
					qWarning()
					    << "Wrong vertices size" << poly.vertices().size();
					return;
				}

				const int pageX = poly.texPage() / 5,
				          pageY = poly.texPage() % 5;

				for (quint8 i = 0; i < 3; ++i) {
					const Vertex &v = poly.vertex(i);
					const TexCoord &tc = poly.texCoord(i);

					QVector3D position((xShift + x + v.x / scaleVect) / scale,
					                   normalizeY(v.y) / scaleVect / scale,
					                   (zShift + z - v.z / scaleVect) / scale);
					QVector2D texcoord;

					if (poly.isRoadTexture()) {
						texcoord.setX((4 * 256 + tc.x) / scaleTexX);
						texcoord.setY((1 * 256 + tc.y) / scaleTexY);
					} else if (poly.isWaterTexture()) {
						texcoord.setX((4 * 256 + tc.x) / scaleTexX);
						texcoord.setY((0 * 256 + tc.y) / scaleTexY);
					} else {
						texcoord.setX((pageX * 256 + tc.x) / scaleTexX);
						texcoord.setY((pageY * 256 + tc.y) / scaleTexY);
					}
					rhiRenderer->bufferVertex(position, color, texcoord);
				}
			}

			xb += 1;

			if (xb >= blocksPerLine) {
				xb = 0;
				yb += 1;
			}
		}

		xs += 1;

		if (xs >= segmentPerLine) {
			xs = 0;
			ys += 1;
		}
	}

	rhiRenderer->commitResourceUpdates(batch);
}

void WorldmapRHIWidget::render(QRhiCommandBuffer *cb)
{
	if (nullptr == _map || rhiRenderer->hasError()) {
		return;
	}

	const QSize pixelSize = renderTarget()->pixelSize();

	if (pixelSize.width() > 0 && pixelSize.height() > 0) {
		_matrixProj.setToIdentity();
		_matrixProj.perspective(
		    70.0f, float(pixelSize.width()) / float(pixelSize.height()),
		    0.000001f, 1000.0f);

		rhiRenderer->setViewport(0, 0, pixelSize.width(), pixelSize.height());
	}

	if (_distance > -0.011124f) {
		_distance = -0.011124f;
	} else if (_distance < -1.78358f) {
		_distance = -1.78358f;
	}

	if (_xTrans > 0.0115338f) {
		_xTrans = 0.0115338f;
	} else if (_xTrans < -1.01512f) {
		_xTrans = -1.01512f;
	}

	if (_yTrans < 0.116807f) {
		_yTrans = 0.116807f;
	} else if (_yTrans > 0.892654f) {
		_yTrans = 0.892654f;
	}

	QMatrix4x4 mModel;
	mModel.translate(_xTrans, _yTrans, _distance);
	mModel.rotate(_xRot, 1.0f, 0.0f, 0.0f);
	mModel.rotate(_yRot, 0.0f, 1.0f, 0.0f);
	mModel.rotate(_zRot, 0.0f, 0.0f, 1.0f);

	QMatrix4x4 mView;

	rhiRenderer->bindProjectionMatrix(_matrixProj);
	rhiRenderer->bindModelMatrix(mModel);
	rhiRenderer->bindViewMatrix(mView);
	rhiRenderer->bindTexture(_megaTexture);

	rhiRenderer->draw(cb, RendererPrimitiveType::PT_TRIANGLES, 1.0f, false);
}

void WorldmapRHIWidget::wheelEvent(QWheelEvent *event)
{
	setFocus();
	_distance += double(event->angleDelta().y()) / 8192.0;
	update();
}

void WorldmapRHIWidget::mousePressEvent(QMouseEvent *event)
{
	setFocus();

	if (event->button() == Qt::MiddleButton) {
		_distance = -0.714248f;
		update();
	} else if (event->button() == Qt::LeftButton) {
		_moveStart = event->position();
	}
}

void WorldmapRHIWidget::mouseReleaseEvent(QMouseEvent *event)
{
	Q_UNUSED(event);
	_moveStart = QPointF();
}

void WorldmapRHIWidget::mouseMoveEvent(QMouseEvent *event)
{
	if (_moveStart.isNull()) {
		return;
	}

	QPointF diff = event->position() - _moveStart;
	bool toUpdate = false;

	if (abs(diff.x()) >= 4.0f) {
		_xTrans += (diff.x() > 0.0f ? 1.0f : -1.0f) / 360.0f;
		toUpdate = true;
	}
	if (abs(diff.y()) >= 4.0f) {
		_yTrans -= (diff.y() > 0.0f ? 1.0f : -1.0f) / 360.0f;
		toUpdate = true;
	}
	if (toUpdate) {
		update();
	}
}

void WorldmapRHIWidget::keyPressEvent(QKeyEvent *event)
{
	if (_lastKeyPressed == event->key()
	    && (event->key() == Qt::Key_Left || event->key() == Qt::Key_Right
	        || event->key() == Qt::Key_Down || event->key() == Qt::Key_Up)) {
		if (_transStep > 100.0f) {
			_transStep *= 0.90f; // accelerator
		}
	} else {
		_transStep = 180.0f;
	}
	_lastKeyPressed = event->key();

	switch (event->key()) {
	case Qt::Key_Left:
		_xTrans += 1.0f / _transStep;
		update();
		break;
	case Qt::Key_Right:
		_xTrans -= 1.0f / _transStep;
		update();
		break;
	case Qt::Key_Down:
		_yTrans += 1.0f / _transStep;
		update();
		break;
	case Qt::Key_Up:
		_yTrans -= 1.0f / _transStep;
		update();
		break;
	case Qt::Key_7:
		_xRot += 0.1f;
		update();
		break;
	case Qt::Key_1:
		_xRot -= 0.1f;
		update();
		break;
	case Qt::Key_8:
		_yRot += 0.1f;
		update();
		break;
	case Qt::Key_2:
		_yRot -= 0.1f;
		update();
		break;
	case Qt::Key_9:
		_zRot += 0.1f;
		update();
		break;
	case Qt::Key_3:
		_zRot -= 0.1f;
		update();
		break;
	default:
		QWidget::keyPressEvent(event);
		return;
	}
}

void WorldmapRHIWidget::focusInEvent(QFocusEvent *event)
{
	grabKeyboard();
	QWidget::focusInEvent(event);
}

void WorldmapRHIWidget::focusOutEvent(QFocusEvent *event)
{
	releaseKeyboard();
	QWidget::focusOutEvent(event);
}

void WorldmapRHIWidget::resizeEvent(QResizeEvent *event)
{
	QRhiWidget::resizeEvent(event);

	if (event->size().isValid()) {
		resizeRHI(event->size().width(), event->size().height());
	}
}

QRgb WorldmapRHIWidget::groundColor(quint8 groundType, quint8 region,
                                    const QSet<quint8> &grounds)
{
	// Original logic from WorldmapGLWidget
	static const QRgb regionColors[8][16] = {
		// Region 0 - Grassland/Forest
		{
		    qRgb(139, 69, 19), // 0: Brown (earth)
		    qRgb(34, 139, 34), // 1: Forest green
		    qRgb(255, 215, 0), // 2: Gold (desert sand)
		    qRgb(70, 130, 180), // 3: Steel blue (water)
		    qRgb(105, 105, 105), // 4: Dim gray (mountain)
		    qRgb(255, 250, 250), // 5: Snow
		    qRgb(160, 82, 45), // 6: Saddle brown (dirt road)
		    qRgb(47, 79, 79), // 7: Dark slate gray (stone)
		    qRgb(85, 107, 47), // 8: Dark olive green (swamp)
		    qRgb(218, 165, 32), // 9: Golden rod (beach sand)
		    qRgb(72, 61, 139), // 10: Dark slate blue (deep water)
		    qRgb(128, 128, 128), // 11: Gray (neutral)
		    qRgb(205, 133, 63), // 12: Peru (clay)
		    qRgb(64, 224, 208), // 13: Turquoise (shallow water)
		    qRgb(46, 125, 50), // 14: Dark green (dense forest)
		    qRgb(139, 69, 19) // 15: Default brown
		},
		// Region 1 - Desert
		{
		    qRgb(194, 178, 128), // 0: Desert sand
		    qRgb(160, 82, 45), // 1: Saddle brown (oasis)
		    qRgb(255, 218, 185), // 2: Peach puff (light sand)
		    qRgb(30, 144, 255), // 3: Dodger blue (water)
		    qRgb(139, 119, 101), // 4: Khaki (rock)
		    qRgb(255, 250, 250), // 5: Snow (mountain peaks)
		    qRgb(205, 133, 63), // 6: Peru (path)
		    qRgb(160, 82, 45), // 7: Saddle brown (stone)
		    qRgb(189, 183, 107), // 8: Dark khaki (wetland)
		    qRgb(255, 228, 181), // 9: Moccasin (fine sand)
		    qRgb(72, 61, 139), // 10: Dark slate blue (deep oasis)
		    qRgb(210, 180, 140), // 11: Tan (neutral desert)
		    qRgb(222, 184, 135), // 12: Burlywood (hardpan)
		    qRgb(175, 238, 238), // 13: Pale turquoise (spring)
		    qRgb(128, 128, 0), // 14: Olive (scrubland)
		    qRgb(194, 178, 128) // 15: Default sand
		},
		// Region 2 - Snow/Ice
		{
		    qRgb(240, 248, 255), // 0: Alice blue (snow)
		    qRgb(25, 25, 112), // 1: Midnight blue (ice caves)
		    qRgb(255, 250, 250), // 2: Snow white
		    qRgb(0, 191, 255), // 3: Deep sky blue (ice water)
		    qRgb(119, 136, 153), // 4: Light slate gray (ice rock)
		    qRgb(255, 255, 255), // 5: Pure white (fresh snow)
		    qRgb(176, 196, 222), // 6: Light steel blue (ice path)
		    qRgb(112, 128, 144), // 7: Slate gray (ice stone)
		    qRgb(95, 158, 160), // 8: Cadet blue (slush)
		    qRgb(230, 230, 250), // 9: Lavender (wind-blown snow)
		    qRgb(25, 25, 112), // 10: Midnight blue (deep ice)
		    qRgb(211, 211, 211), // 11: Light gray (dirty snow)
		    qRgb(192, 192, 192), // 12: Silver (ice)
		    qRgb(175, 238, 238), // 13: Pale turquoise (meltwater)
		    qRgb(72, 61, 139), // 14: Dark slate blue (crevasse)
		    qRgb(240, 248, 255) // 15: Default snow
		},
		// Region 3 - Volcanic
		{
		    qRgb(139, 0, 0), // 0: Dark red (volcanic soil)
		    qRgb(255, 69, 0), // 1: Red orange (lava)
		    qRgb(105, 105, 105), // 2: Dim gray (ash)
		    qRgb(25, 25, 112), // 3: Midnight blue (volcanic lake)
		    qRgb(85, 85, 85), // 4: Dim gray (volcanic rock)
		    qRgb(255, 140, 0), // 5: Dark orange (sulfur)
		    qRgb(128, 0, 0), // 6: Maroon (burnt path)
		    qRgb(47, 79, 79), // 7: Dark slate gray (obsidian)
		    qRgb(102, 51, 153), // 8: Rebecca purple (toxic pool)
		    qRgb(255, 215, 0), // 9: Gold (sulfur deposits)
		    qRgb(139, 0, 139), // 10: Dark magenta (deep lava)
		    qRgb(169, 169, 169), // 11: Dark gray (neutral)
		    qRgb(165, 42, 42), // 12: Brown (burnt earth)
		    qRgb(255, 99, 71), // 13: Tomato (hot springs)
		    qRgb(255, 0, 0), // 14: Red (active lava)
		    qRgb(139, 0, 0) // 15: Default volcanic
		},
		// Regions 4-7: Extended palettes for different biomes
		// Region 4 - Coastal
		{
		    qRgb(238, 203, 173), // 0: Bisque (beach)
		    qRgb(46, 125, 50), // 1: Dark green (coastal forest)
		    qRgb(255, 218, 185), // 2: Peach puff (sand dunes)
		    qRgb(30, 144, 255), // 3: Dodger blue (ocean)
		    qRgb(112, 128, 144), // 4: Slate gray (cliff)
		    qRgb(255, 250, 250), // 5: Snow (high cliffs)
		    qRgb(160, 82, 45), // 6: Saddle brown (boardwalk)
		    qRgb(105, 105, 105), // 7: Dim gray (jetty)
		    qRgb(64, 224, 208), // 8: Turquoise (lagoon)
		    qRgb(255, 228, 196), // 9: Bisque (shell beach)
		    qRgb(0, 100, 0), // 10: Dark green (seaweed)
		    qRgb(176, 196, 222), // 11: Light steel blue (neutral)
		    qRgb(205, 133, 63), // 12: Peru (tide pools)
		    qRgb(127, 255, 212), // 13: Aquamarine (shallow bay)
		    qRgb(72, 61, 139), // 14: Dark slate blue (deep sea)
		    qRgb(238, 203, 173) // 15: Default coastal
		},
		// Region 5 - Swamp
		{
		    qRgb(85, 107, 47), // 0: Dark olive green (swamp)
		    qRgb(34, 139, 34), // 1: Forest green (swamp trees)
		    qRgb(160, 82, 45), // 2: Saddle brown (mud)
		    qRgb(47, 79, 79), // 3: Dark slate gray (murky water)
		    qRgb(105, 105, 105), // 4: Dim gray (dead trees)
		    qRgb(240, 248, 255), // 5: Alice blue (mist)
		    qRgb(139, 69, 19), // 6: Saddle brown (walkway)
		    qRgb(128, 128, 128), // 7: Gray (stone)
		    qRgb(107, 142, 35), // 8: Olive drab (bog)
		    qRgb(189, 183, 107), // 9: Dark khaki (marsh grass)
		    qRgb(25, 25, 112), // 10: Midnight blue (deep bog)
		    qRgb(128, 128, 0), // 11: Olive (neutral swamp)
		    qRgb(154, 205, 50), // 12: Yellow green (algae)
		    qRgb(64, 224, 208), // 13: Turquoise (clear pool)
		    qRgb(0, 100, 0), // 14: Dark green (dense swamp)
		    qRgb(85, 107, 47) // 15: Default swamp
		},
		// Region 6 - Highlands
		{
		    qRgb(160, 82, 45), // 0: Saddle brown (highland soil)
		    qRgb(34, 139, 34), // 1: Forest green (pine forest)
		    qRgb(218, 165, 32), // 2: Golden rod (autumn grass)
		    qRgb(70, 130, 180), // 3: Steel blue (mountain lake)
		    qRgb(105, 105, 105), // 4: Dim gray (granite)
		    qRgb(255, 250, 250), // 5: Snow (peaks)
		    qRgb(139, 69, 19), // 6: Saddle brown (trail)
		    qRgb(112, 128, 144), // 7: Slate gray (stone)
		    qRgb(143, 188, 143), // 8: Dark sea green (alpine meadow)
		    qRgb(240, 230, 140), // 9: Khaki (highland grass)
		    qRgb(72, 61, 139), // 10: Dark slate blue (deep tarn)
		    qRgb(169, 169, 169), // 11: Dark gray (neutral)
		    qRgb(205, 133, 63), // 12: Peru (scree)
		    qRgb(175, 238, 238), // 13: Pale turquoise (spring)
		    qRgb(85, 107, 47), // 14: Dark olive green (heather)
		    qRgb(160, 82, 45) // 15: Default highland
		},
		// Region 7 - Wasteland
		{
		    qRgb(128, 128, 128), // 0: Gray (wasteland)
		    qRgb(85, 85, 85), // 1: Dim gray (dead vegetation)
		    qRgb(205, 133, 63), // 2: Peru (dust)
		    qRgb(47, 79, 79), // 3: Dark slate gray (polluted water)
		    qRgb(105, 105, 105), // 4: Dim gray (rubble)
		    qRgb(211, 211, 211), // 5: Light gray (ash)
		    qRgb(139, 69, 19), // 6: Saddle brown (dirt track)
		    qRgb(112, 128, 144), // 7: Slate gray (broken stone)
		    qRgb(128, 0, 128), // 8: Purple (toxic waste)
		    qRgb(169, 169, 169), // 9: Dark gray (debris)
		    qRgb(25, 25, 112), // 10: Midnight blue (tar pit)
		    qRgb(128, 128, 128), // 11: Gray (neutral wasteland)
		    qRgb(160, 82, 45), // 12: Saddle brown (rust)
		    qRgb(102, 51, 153), // 13: Rebecca purple (chemical spill)
		    qRgb(85, 85, 85), // 14: Dim gray (blighted area)
		    qRgb(128, 128, 128) // 15: Default wasteland
		}
	};

	region = qMin(region, quint8(7));
	groundType = qMin(groundType, quint8(15));

	if (!grounds.isEmpty()) {
		if (grounds.contains(groundType)) {
			QRgb baseColor = regionColors[region][groundType];

			int r = qRed(baseColor);
			int g = qGreen(baseColor);
			int b = qBlue(baseColor);

			r = qMin(255, static_cast<int>(r * 1.3));
			g = qMin(255, static_cast<int>(g * 1.3));
			b = qMin(255, static_cast<int>(b * 1.3));

			return qRgb(r, g, b);
		} else {
			QRgb baseColor = regionColors[region][groundType];

			int r = qRed(baseColor);
			int g = qGreen(baseColor);
			int b = qBlue(baseColor);

			r = static_cast<int>(r * 0.7);
			g = static_cast<int>(g * 0.7);
			b = static_cast<int>(b * 0.7);

			return qRgb(r, g, b);
		}
	}

	return regionColors[region][groundType];
}

void WorldmapRHIWidget::resizeRHI(int width, int height)
{
	if (rhiRenderer != nullptr) {
		rhiRenderer->setViewport(0, 0, width, height);
	}

	_matrixProj.setToIdentity();
	_matrixProj.perspective(70.0f, float(width) / float(height), 0.000001f,
	                        1000.0f);
}
