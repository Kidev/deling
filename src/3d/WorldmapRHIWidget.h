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
#include "game/worldmap/Map.h"
#include "Renderer.h"

class WorldmapRHIWidget : public QRhiWidget
{
	Q_OBJECT
public:
	explicit WorldmapRHIWidget(QWidget *parent = nullptr,
	                           Qt::WindowFlags f = Qt::WindowFlags());

	virtual ~WorldmapRHIWidget();
	void setMap(Map *map);
	inline const Map *map() const
	{
		return _map;
	}
	void setLimits(const QRect &rect);
	void setXTrans(float trans);
	inline float xTrans() const
	{
		return _xTrans;
	}
	void setYTrans(float trans);
	inline float yTrans() const
	{
		return _yTrans;
	}
	void setZTrans(float trans);
	inline float zTrans() const
	{
		return _distance;
	}
	void setXRot(float rot);
	inline float xRot() const
	{
		return _xRot;
	}
	void setYRot(float rot);
	inline float yRot() const
	{
		return _yRot;
	}
	void setZRot(float rot);
	inline float zRot() const
	{
		return _zRot;
	}
	inline int texture() const
	{
		return _texture;
	}
	inline int segmentGroupId() const
	{
		return _segmentGroupId;
	}
	inline int segmentId() const
	{
		return _segmentId;
	}
	inline int blockId() const
	{
		return _blockId;
	}
	inline int polyId() const
	{
		return _polyId;
	}
	inline int clutId() const
	{
		return _clutId;
	}
	inline int groundType() const
	{
		return _groundType;
	}
	QRgb groundColor(quint8 groundType, quint8 region,
	                 const QSet<quint8> &grounds);
	void resizeRHI(int width, int height);
public slots:
	void resetCamera();
	void setTexture(int texture);
	void setSegmentGroupId(int segmentGroupId);
	void setSegmentId(int segmentId);
	void setBlockId(int blockId);
	void setGroundType(int groundType);
	void setPolyId(int polyId);
	void setClutId(int clutId);
	void setSegmentFiltering(Map::SegmentFiltering filtering);
	void dumpCurrent();

protected:
	virtual void initialize(QRhiCommandBuffer *cb) override;
	virtual void render(QRhiCommandBuffer *cb) override;
	virtual void releaseResources() override;
	virtual void wheelEvent(QWheelEvent *event) override;
	virtual void mousePressEvent(QMouseEvent *event) override;
	virtual void mouseMoveEvent(QMouseEvent *event) override;
	virtual void mouseReleaseEvent(QMouseEvent *event) override;
	virtual void keyPressEvent(QKeyEvent *event) override;
	virtual void focusInEvent(QFocusEvent *event) override;
	virtual void focusOutEvent(QFocusEvent *event) override;
	virtual void resizeEvent(QResizeEvent *event) override;

private:
	void importVertices();

	Map *_map;
	float _distance;
	float _xRot, _yRot, _zRot;
	float _xTrans, _yTrans, _transStep;
	int _lastKeyPressed, _texture, _segmentGroupId, _segmentId, _blockId;
	int _groundType, _polyId, _clutId;
	QRect _limits;
	QPointF _moveStart;
	QRhiTexture *_megaTexture;
	Renderer *rhiRenderer;
	QMatrix4x4 _matrixProj;
	Map::SegmentFiltering _segmentFiltering;
	bool _initialized;
};
