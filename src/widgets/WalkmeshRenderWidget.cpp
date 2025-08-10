/****************************************************************************
 * * Deling QRhi port
 ** Copyright (C) 2025 Alexandre 'kidev' Poumaroux <public@kidev.org>
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

#include "WalkmeshRenderWidget.h"
#include <QMatrix4x4>
#include <QKeyEvent>

// Helper to load a QShader from resource
static QShader loadShader(const QString &name)
{
	QFile f(name);
	if (!f.open(QIODevice::ReadOnly)) {
		qWarning("Failed to open shader file %s", qPrintable(name));
		return QShader();
	}
	return QShader::fromSerialized(f.readAll());
}

void WalkmeshRenderWidget::rebuildBackgroundResources(QRhiCommandBuffer *cb)
{
	// Destroy previous bg texture/SRB
	m_bgTexture.reset();
	m_bgSrb.reset();

	QRhiResourceUpdateBatch *rub = m_rhi->nextResourceUpdateBatch();

	if (!m_bgImage.isNull()) {
		// Create a texture from the actual background image
		m_bgTexture.reset(m_rhi->newTexture(
		    QRhiTexture::RGBA8, m_bgImage.size(), 1,
		    QRhiTexture::MipMapped | QRhiTexture::UsedAsTransferSource));
		m_bgTexture->create();
		rub->uploadTexture(m_bgTexture.get(), m_bgImage);
	} else {
		// Create a 1x1 fallback texture (black) so SRB is never null
		QImage dummy(1, 1, QImage::Format_RGBA8888);
		dummy.fill(Qt::black);
		m_bgTexture.reset(m_rhi->newTexture(QRhiTexture::RGBA8, dummy.size(), 1,
		                                    QRhiTexture::UsedAsTransferSource));
		m_bgTexture->create();
		rub->uploadTexture(m_bgTexture.get(), dummy);
	}

	// Ensure we have a sampler
	if (!m_bgSampler) {
		m_bgSampler.reset(m_rhi->newSampler(
		    QRhiSampler::Linear, QRhiSampler::Linear, QRhiSampler::None,
		    QRhiSampler::ClampToEdge, QRhiSampler::ClampToEdge));
		m_bgSampler->create();
	}

	// Create SRB with sampler at binding 1
	m_bgSrb.reset(m_rhi->newShaderResourceBindings());
	m_bgSrb->setBindings({ QRhiShaderResourceBinding::sampledTexture(
	    1, QRhiShaderResourceBinding::FragmentStage, m_bgTexture.get(),
	    m_bgSampler.get()) });
	m_bgSrb->create();

	cb->resourceUpdate(rub);

	m_bgDirty = false;
}

WalkmeshRenderWidget::WalkmeshRenderWidget(QWidget *parent) : QRhiWidget(parent)
{
	setFocusPolicy(
	    Qt::StrongFocus); // to receive key events, hopefully before the tabs
}

WalkmeshRenderWidget::~WalkmeshRenderWidget()
{
	this->resetResources();
}

void WalkmeshRenderWidget::setXRotation(int angle)
{
	angle %= 360;
	if (angle < 0)
		angle += 360;
	if (qFuzzyCompare(m_xRot, float(angle)))
		return;
	m_xRot = float(angle);
	update();
}

void WalkmeshRenderWidget::setYRotation(int angle)
{
	angle %= 360;
	if (angle < 0)
		angle += 360;
	if (qFuzzyCompare(m_yRot, float(angle)))
		return;
	m_yRot = float(angle);
	update();
}

void WalkmeshRenderWidget::setZRotation(int angle)
{
	angle %= 360;
	if (angle < 0)
		angle += 360;
	if (qFuzzyCompare(m_zRot, float(angle)))
		return;
	m_zRot = float(angle);
	update();
}

void WalkmeshRenderWidget::setCurrentTabIndex(int index)
{
	_selectedTabIndex = index;
	update();
}

void WalkmeshRenderWidget::resetCamera()
{
	m_xRot = m_yRot = m_zRot = 0.0f;
	m_xTrans = m_yTrans = 0.0f;
	update();
}

void WalkmeshRenderWidget::setBackgroundVisible(bool show)
{
	m_backgroundVisible = show;
	update();
}

void WalkmeshRenderWidget::clear()
{
	m_fieldData = nullptr;
	m_bgImage = QImage();
	_drawLine = false;
	update();
}

void WalkmeshRenderWidget::setSelectedTriangle(int triangle)
{
	_selectedTriangle = triangle;
	update();
}

void WalkmeshRenderWidget::setSelectedDoor(int door)
{
	_selectedDoor = door;
	update();
}

void WalkmeshRenderWidget::setSelectedGate(int gate)
{
	_selectedGate = gate;
	update();
}

void WalkmeshRenderWidget::setLineToDraw(const Vertex vertex[2])
{
	_lineToDrawPoint1 = vertex[0];
	_lineToDrawPoint2 = vertex[1];
	_drawLine = true;
	update();
}

void WalkmeshRenderWidget::fill(Field *data)
{
	m_fieldData = data;

	// Load background image
	if (m_fieldData && m_fieldData->getBackgroundFile()) {
		m_bgImage = m_fieldData->getBackgroundFile()->background();
	} else {
		m_bgImage = QImage();
	}

	updatePerspective();
	resetCamera();

	m_bgDirty = true;

	update();
}

void WalkmeshRenderWidget::computeFov()
{
	if (m_fieldData && m_fieldData->hasCaFile()
	    && camID < m_fieldData->getCaFile()->cameraCount()) {
		const Camera &cam = m_fieldData->getCaFile()->camera(camID);
		// FF8 camera_zoom to vertical FOV conversion:
		// Using formula: fov = 2 * atan(screenHeight/(2*focalLength))
		fovy = (2.0 * atan(240.0 / double(cam.camera_zoom) / 2.0))
		       * (180.0 / M_PI);
	} else {
		fovy = 70.0;
	}
}

void WalkmeshRenderWidget::setCurrentFieldCamera(int cam)
{
	camID = cam;
	updatePerspective();
}

void WalkmeshRenderWidget::updatePerspective()
{
	computeFov();
	if (rhi() && window()) {
		// Recreate projection matrix on next render
		resize(width(), height());
	}
	update();
}

void WalkmeshRenderWidget::keyPressEvent(QKeyEvent *event)
{
	const float step = 10.0f;
	bool handled = true;
	switch (event->key()) {
	case Qt::Key_Left:
		m_xTrans -= step;
		break;
	case Qt::Key_Right:
		m_xTrans += step;
		break;
	case Qt::Key_Up:
		m_yTrans += step;
		break;
	case Qt::Key_Down:
		m_yTrans -= step;
		break;
	default:
		handled = false;
		break;
	}
	if (handled) {
		update();
	} else {
		QRhiWidget::keyPressEvent(event);
	}
}

// QRhi initialization: create pipelines and buffers
void WalkmeshRenderWidget::initialize(QRhiCommandBuffer *cb)
{
	if (m_rhi != rhi()) {
		m_rhi = rhi();
		m_pipeline.reset();
		m_bgPipeline.reset();
		m_linePipeline.reset();
		m_wireVbuf.reset();
		m_exitsVbuf.reset();
		m_doorsVbuf.reset();
		m_bgVbuf.reset();
		m_ubuf.reset();
		m_srb.reset();
		m_bgSrb.reset();
		m_bgTexture.reset();
		m_bgSampler.reset();
		m_outerEdges.clear();
		m_outerEdgesReady = false;
	}
	if (m_pipeline) { // already initialized
		return;
	}

	// Uniform buffer (MVP)
	m_ubuf.reset(
	    m_rhi->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::UniformBuffer, 64));
	m_ubuf->create();

	// Dynamic vertex buffers (rebuilt every frame)
	const int triCountInit = (m_fieldData && m_fieldData->hasIdFile())
	                             ? m_fieldData->getIdFile()->triangleCount()
	                             : 0;
	const int wireVertsInit = qMax(1, triCountInit) * 6; // 3 edges * 2 verts

	m_wireVbuf.reset(m_rhi->newBuffer(QRhiBuffer::Dynamic,
	                                  QRhiBuffer::VertexBuffer,
	                                  wireVertsInit * (6 * sizeof(float))));
	m_wireVbuf->create();

	// exits / doors / mesh (lines) get reasonable init, we grow if needed
	m_exitsVbuf.reset(
	    m_rhi->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::VertexBuffer,
	                     1024 * 2 * 6 * sizeof(float))); // guess: 1024 lines
	m_exitsVbuf->create();

	m_doorsVbuf.reset(m_rhi->newBuffer(QRhiBuffer::Dynamic,
	                                   QRhiBuffer::VertexBuffer,
	                                   1024 * 2 * 6 * sizeof(float)));
	m_doorsVbuf->create();

	// SRB for MVP at binding 0 (vertex stage)
	m_srb.reset(m_rhi->newShaderResourceBindings());
	m_srb->setBindings({ QRhiShaderResourceBinding::uniformBuffer(
	    0, QRhiShaderResourceBinding::VertexStage, m_ubuf.get()) });
	m_srb->create();

	// Triangles pipeline
	m_pipeline.reset(m_rhi->newGraphicsPipeline());
	m_pipeline->setDepthTest(true);
	m_pipeline->setDepthWrite(true);
	m_pipeline->setCullMode(QRhiGraphicsPipeline::None);
	m_pipeline->setShaderStages(
	    { { QRhiShaderStage::Vertex,
	        loadShader(":/src/qt/shaders/walkmesh.vert.qsb") },
	      { QRhiShaderStage::Fragment,
	        loadShader(":/src/qt/shaders/walkmesh.frag.qsb") } });
	{
		QRhiVertexInputLayout layout;
		layout.setBindings({ { 6 * sizeof(float) } });
		layout.setAttributes(
		    { { 0, 0, QRhiVertexInputAttribute::Float3, 0 },
		      { 0, 1, QRhiVertexInputAttribute::Float3, 3 * sizeof(float) } });
		m_pipeline->setVertexInputLayout(layout);
	}
	m_pipeline->setShaderResourceBindings(m_srb.get());
	m_pipeline->setRenderPassDescriptor(renderTarget()->renderPassDescriptor());
	m_pipeline->setTopology(QRhiGraphicsPipeline::Triangles);
	m_pipeline->create();

	// Background quad VBO (x,y,u,v)
	static const float bgQuadData[6 * 4] = { -1.0f, -1.0f, 0.0f,  1.0f,  1.0f,
		                                     -1.0f, 1.0f,  1.0f,  1.0f,  1.0f,
		                                     1.0f,  0.0f,  -1.0f, -1.0f, 0.0f,
		                                     1.0f,  1.0f,  1.0f,  1.0f,  0.0f,
		                                     -1.0f, 1.0f,  0.0f,  0.0f };
	m_bgVbuf.reset(m_rhi->newBuffer(
	    QRhiBuffer::Immutable, QRhiBuffer::VertexBuffer, sizeof(bgQuadData)));
	m_bgVbuf->create();
	{
		QRhiResourceUpdateBatch *initBatch = m_rhi->nextResourceUpdateBatch();
		initBatch->uploadStaticBuffer(m_bgVbuf.get(), bgQuadData);
		cb->resourceUpdate(initBatch);
	}

	// Fallback BG sampler/texture/SRB (replaced when real image set)
	if (!m_bgSampler) {
		m_bgSampler.reset(m_rhi->newSampler(
		    QRhiSampler::Linear, QRhiSampler::Linear, QRhiSampler::None,
		    QRhiSampler::ClampToEdge, QRhiSampler::ClampToEdge));
		m_bgSampler->create();
	}
	{
		QImage dummy(1, 1, QImage::Format_RGBA8888);
		dummy.fill(Qt::black);
		m_bgTexture.reset(m_rhi->newTexture(QRhiTexture::RGBA8, dummy.size(), 1,
		                                    QRhiTexture::UsedAsTransferSource));
		m_bgTexture->create();
		QRhiResourceUpdateBatch *rub = m_rhi->nextResourceUpdateBatch();
		rub->uploadTexture(m_bgTexture.get(), dummy);
		cb->resourceUpdate(rub);

		m_bgSrb.reset(m_rhi->newShaderResourceBindings());
		m_bgSrb->setBindings({ QRhiShaderResourceBinding::sampledTexture(
		    1, QRhiShaderResourceBinding::FragmentStage, m_bgTexture.get(),
		    m_bgSampler.get()) });
		m_bgSrb->create();
	}

	// Background pipeline
	m_bgPipeline.reset(m_rhi->newGraphicsPipeline());
	m_bgPipeline->setDepthTest(false);
	m_bgPipeline->setDepthWrite(false);
	m_bgPipeline->setCullMode(QRhiGraphicsPipeline::None);
	m_bgPipeline->setTopology(QRhiGraphicsPipeline::Triangles);
	m_bgPipeline->setShaderStages(
	    { { QRhiShaderStage::Vertex,
	        loadShader(":/src/qt/shaders/background.vert.qsb") },
	      { QRhiShaderStage::Fragment,
	        loadShader(":/src/qt/shaders/background.frag.qsb") } });
	{
		QRhiVertexInputLayout bgLayout;
		bgLayout.setBindings({ { 4 * sizeof(float) } });
		bgLayout.setAttributes(
		    { { 0, 0, QRhiVertexInputAttribute::Float2, 0 },
		      { 0, 1, QRhiVertexInputAttribute::Float2, 2 * sizeof(float) } });
		m_bgPipeline->setVertexInputLayout(bgLayout);
	}
	m_bgPipeline->setShaderResourceBindings(m_bgSrb.get());
	m_bgPipeline->setRenderPassDescriptor(
	    renderTarget()->renderPassDescriptor());
	m_bgPipeline->create();

	// Lines pipeline (wireframe + exits + doors + border)
	m_linePipeline.reset(m_rhi->newGraphicsPipeline());
	m_linePipeline->setDepthTest(false);
	m_linePipeline->setDepthWrite(false);
	m_linePipeline->setCullMode(QRhiGraphicsPipeline::None);
	m_linePipeline->setTopology(QRhiGraphicsPipeline::Lines);
	m_linePipeline->setShaderStages(
	    { { QRhiShaderStage::Vertex,
	        loadShader(":/src/qt/shaders/walkmesh.vert.qsb") },
	      { QRhiShaderStage::Fragment,
	        loadShader(":/src/qt/shaders/walkmesh.frag.qsb") } });
	{
		QRhiVertexInputLayout lyt;
		lyt.setBindings({ { 6 * sizeof(float) } });
		lyt.setAttributes(
		    { { 0, 0, QRhiVertexInputAttribute::Float3, 0 },
		      { 0, 1, QRhiVertexInputAttribute::Float3, 3 * sizeof(float) } });
		m_linePipeline->setVertexInputLayout(lyt);
	}
	m_linePipeline->setShaderResourceBindings(m_srb.get());
	m_linePipeline->setRenderPassDescriptor(
	    renderTarget()->renderPassDescriptor());
	m_linePipeline->create();
}

// Take a deep breath, and lets go
void WalkmeshRenderWidget::render(QRhiCommandBuffer *cb)
{
	// Clear if no data
	if (!m_fieldData) {
		cb->beginPass(renderTarget(),
		              m_backgroundVisible
		                  ? Qt::black
		                  : QColor::fromRgbF(0.2f, 0.2f, 0.2f, 1.0f),
		              { 1.0f, 0 }, nullptr);
		cb->endPass();
		return;
	}

	// Ensure background SRB/texture is valid
	if (m_bgDirty || !m_bgSrb || !m_bgTexture) {
		rebuildBackgroundResources(cb);
	}

	const int triCount = m_fieldData->getIdFile()->triangleCount();

	// STAGE 1: EDGE ACCUMULATION (dedupe) for WIREFRAME + BORDER + TRIANGLES
	struct EdgeAccum {
		Vertex a;
		Vertex b;
		int count = 0; // how many triangles reference this edge
		bool highlight = false; // true if any owning triangle is selected
	};

	auto keyPoint = [](const Vertex &v) -> QString {
		return QString::number(double(v.x), 'f', 6) + QLatin1Char(',')
		       + QString::number(double(v.y), 'f', 6) + QLatin1Char(',')
		       + QString::number(double(v.z), 'f', 6);
	};
	auto keyEdge = [&](const Vertex &p, const Vertex &q) -> QString {
		const QString kp = keyPoint(p);
		const QString kq = keyPoint(q);
		return (kp < kq) ? (kp + QLatin1Char('|') + kq)
		                 : (kq + QLatin1Char('|') + kp);
	};

	QHash<QString, EdgeAccum> edgeMap;
	edgeMap.reserve(triCount * 3);

	auto addEdge = [&](const Vertex &p, const Vertex &q, bool triSelected) {
		const QString k = keyEdge(p, q);
		auto it = edgeMap.find(k);
		if (it == edgeMap.end()) {
			EdgeAccum e;
			e.a = p;
			e.b = q;
			e.count = 1;
			e.highlight = triSelected;
			edgeMap.insert(k, e);
		} else {
			it->count += 1;
			if (triSelected)
				it->highlight = true;
		}
	};

	for (int ti = 0; ti < triCount; ++ti) {
		const Triangle &tri = m_fieldData->getIdFile()->triangle(ti);
		const bool selectedTri = (ti == _selectedTriangle);

		const Vertex v0 = IdFile::toVertex_s(tri.vertices[0]);
		const Vertex v1 = IdFile::toVertex_s(tri.vertices[1]);
		const Vertex v2 = IdFile::toVertex_s(tri.vertices[2]);

		addEdge(v0, v1, selectedTri);
		addEdge(v1, v2, selectedTri);
		addEdge(v2, v0, selectedTri);
	}

	// Compute once "outer edges at load", using this first valid edgeMap
	if (!m_outerEdgesReady && !edgeMap.isEmpty()) {
		m_outerEdges.clear();
		for (auto it = edgeMap.constBegin(); it != edgeMap.constEnd(); ++it) {
			if (it.value().count == 1) {
				m_outerEdges.insert(it.key());
			}
		}
		m_outerEdgesReady = true;
	}

	// STAGE 2: SINGLE-PASS BUILD of WIREFRAME with SEL_TRI > BORDER > MESH
	struct LineV {
		float x, y, z, r, g, b;
	};
	QVector<LineV> white;
	white.reserve(edgeMap.size() * 2);
	QVector<LineV> cyan;
	cyan.reserve(edgeMap.size() / 2 * 2);
	QVector<LineV> orange;
	orange.reserve(6); // a few at most

	auto pushEdge = [](QVector<LineV> &dst, const Vertex &a, const Vertex &b,
	                   float r, float g, float bcol) {
		// a
		dst.push_back(LineV{ float(a.x), float(a.y), float(a.z), r, g, bcol });
		// b
		dst.push_back(LineV{ float(b.x), float(b.y), float(b.z), r, g, bcol });
	};

	for (auto it = edgeMap.constBegin(); it != edgeMap.constEnd(); ++it) {
		const EdgeAccum &e = it.value();
		const bool wasOuterAtLoad = m_outerEdges.contains(it.key());

		if (e.highlight) {
			// Selected triangle owns this edge -> ORANGE
			pushEdge(orange, e.a, e.b, 1.0f, 0.5f, 0.0f);
		} else if (wasOuterAtLoad) {
			// Outer rim -> CYAN (frozen at load time)
			pushEdge(cyan, e.a, e.b, 0.0f, 1.0f, 1.0f);
		} else {
			// Internal shared edge -> WHITE
			pushEdge(white, e.a, e.b, 1.0f, 1.0f, 1.0f);
		}
	}

	// Concatenate in priority order: WHITE first (lowest), CYAN, ORANGE
	const int totalVerts = white.size() + cyan.size() + orange.size();
	QByteArray wireData;
	wireData.resize(totalVerts * int(sizeof(LineV)));
	{
		float *dst = reinterpret_cast<float *>(wireData.data());
		auto dump = [&](const QVector<LineV> &v) {
			const LineV *ptr = v.constData();
			const int n = v.size() * int(sizeof(LineV)) / int(sizeof(float));
			memcpy(dst, ptr, v.size() * sizeof(LineV));
			dst += n;
		};
		dump(white);
		dump(cyan);
		dump(orange);
	}
	m_wireVertexCount = totalVerts;

	// STAGE 3: SINGLE-PASS BUILD of EXITS and DOORS
	QByteArray exitsData, doorsData;
	m_exitsVertexCount = 0;
	m_doorsVertexCount = 0;
	if (m_fieldData->hasInfFile()) {
		const auto *inf = m_fieldData->getInfFile();
		// Exits (red)
		// PS: Calling exits in UI and gateway here => fy :)
		{
			const int n = inf->gatewayCount();
			exitsData.resize(n * 2 * 6 * int(sizeof(float)));
			float *eptr = reinterpret_cast<float *>(exitsData.data());
			for (int i = 0; i < n; ++i) {
				const auto &gw = inf->getGateway(i);
				// p0
				*eptr++ = float(gw.exitLine[0].x);
				*eptr++ = float(gw.exitLine[0].y);
				*eptr++ = float(gw.exitLine[0].z);
				*eptr++ = 1.0f;
				*eptr++ = 0.0f;
				*eptr++ = 0.0f;
				// p1
				*eptr++ = float(gw.exitLine[1].x);
				*eptr++ = float(gw.exitLine[1].y);
				*eptr++ = float(gw.exitLine[1].z);
				*eptr++ = 1.0f;
				*eptr++ = 0.0f;
				*eptr++ = 0.0f;
			}
			m_exitsVertexCount = n * 2;
		}
		// Doors (green)
		// PS: Calling doors in UI and trigger here => fy again :)
		{
			const int n = inf->triggerCount();
			doorsData.resize(n * 2 * 6 * int(sizeof(float)));
			float *dptr = reinterpret_cast<float *>(doorsData.data());
			for (int i = 0; i < n; ++i) {
				const auto &tr = inf->getTrigger(i);
				// p0
				*dptr++ = float(tr.trigger_line[0].x);
				*dptr++ = float(tr.trigger_line[0].y);
				*dptr++ = float(tr.trigger_line[0].z);
				*dptr++ = 0.0f;
				*dptr++ = 1.0f;
				*dptr++ = 0.0f;
				// p1
				*dptr++ = float(tr.trigger_line[1].x);
				*dptr++ = float(tr.trigger_line[1].y);
				*dptr++ = float(tr.trigger_line[1].z);
				*dptr++ = 0.0f;
				*dptr++ = 1.0f;
				*dptr++ = 0.0f;
			}
			m_doorsVertexCount = n * 2;
		}
	}

	// STAGE 4: SINGLE-PASS BUILD of SELECTION SQUARES
	QByteArray markersData;
	{
		QVector<QVector3D> pts;
		QVector<QVector3D> cols;

		// We always display the sel squares of triangle no matter the tab
		if (_selectedTriangle >= 0 && _selectedTriangle < triCount) {
			const Triangle &tri =
			    m_fieldData->getIdFile()->triangle(_selectedTriangle);
			const Vertex v0 = IdFile::toVertex_s(tri.vertices[0]);
			const Vertex v1 = IdFile::toVertex_s(tri.vertices[1]);
			const Vertex v2 = IdFile::toVertex_s(tri.vertices[2]);
			const QVector3D col(1.0f, 0.5f, 0.0f);
			pts << QVector3D(v0.x, v0.y, v0.z) << QVector3D(v1.x, v1.y, v1.z)
			    << QVector3D(v2.x, v2.y, v2.z);
			cols << col << col << col;
		}
		if (m_fieldData->hasInfFile()) {
			const auto *inf = m_fieldData->getInfFile();
			// Usage of isExitsTabSelected() make disp only in the exits tab
			if (isExitsTabSelected() && _selectedGate >= 0
			    && _selectedGate < inf->gatewayCount()) {
				const auto &gw = inf->getGateway(_selectedGate);
				const QVector3D col(1.0f, 0.0f, 0.0f);
				pts << QVector3D(gw.exitLine[0].x, gw.exitLine[0].y,
				                 gw.exitLine[0].z)
				    << QVector3D(gw.exitLine[1].x, gw.exitLine[1].y,
				                 gw.exitLine[1].z);
				cols << col << col;
			}
			// Usage of isExitsTabSelected() make disp only in the doors tab
			if (isDoorsTabSelected() && _selectedDoor >= 0
			    && _selectedDoor < inf->triggerCount()) {
				const auto &tr = inf->getTrigger(_selectedDoor);
				const QVector3D col(0.0f, 1.0f, 0.0f);
				pts << QVector3D(tr.trigger_line[0].x, tr.trigger_line[0].y,
				                 tr.trigger_line[0].z)
				    << QVector3D(tr.trigger_line[1].x, tr.trigger_line[1].y,
				                 tr.trigger_line[1].z);
				cols << col << col;
			}
		}

		QVector3D right(1, 0, 0), up(0, 1, 0);
		if (m_fieldData->hasCaFile()
		    && camID < m_fieldData->getCaFile()->cameraCount()) {
			const Camera &cam = m_fieldData->getCaFile()->camera(camID);
			right = QVector3D(cam.camera_axis[0].x, cam.camera_axis[0].y,
			                  cam.camera_axis[0].z);
			up = QVector3D(cam.camera_axis[1].x, cam.camera_axis[1].y,
			               cam.camera_axis[1].z);
		}

		auto sane = [](QVector3D v) -> QVector3D {
			if (!qIsFinite(v.x()) || !qIsFinite(v.y()) || !qIsFinite(v.z()))
				return QVector3D(0, 0, 0);
			if (v.lengthSquared() < 1e-12f)
				return QVector3D(0, 0, 0);
			v.normalize();
			return v;
		};
		right = sane(right);
		if (right.isNull())
			right = QVector3D(1, 0, 0);
		up = sane(up);
		if (up.isNull())
			up = QVector3D(0, 1, 0);

		const float s = 10.0f; // half-size of squares in world units
		markersData.resize(pts.size()
		                   * 6 /*verts*/ * 6 /*floats*/ * int(sizeof(float)));
		float *mptr = reinterpret_cast<float *>(markersData.data());
		auto putV = [&](const QVector3D &p, const QVector3D &c) {
			*mptr++ = p.x();
			*mptr++ = p.y();
			*mptr++ = p.z();
			*mptr++ = c.x();
			*mptr++ = c.y();
			*mptr++ = c.z();
		};
		for (int i = 0; i < pts.size(); ++i) {
			const QVector3D c = pts[i];
			const QVector3D col = cols[i];
			const QVector3D a = c + (-s * right) + (-s * up);
			const QVector3D b = c + (s * right) + (-s * up);
			const QVector3D d = c + (s * right) + (s * up);
			const QVector3D e = c + (-s * right) + (s * up);
			putV(a, col);
			putV(b, col);
			putV(d, col);
			putV(a, col);
			putV(d, col);
			putV(e, col);
		}
		m_markersVertexCount = pts.size() * 6;
	}

	// STAGE 5: UPLOAD the RESOURCES UPDATE (grow buffers if needed)
	QRhiResourceUpdateBatch *rub = m_rhi->nextResourceUpdateBatch();
	auto ensureUpdate = [&](std::unique_ptr<QRhiBuffer> &buf,
	                        const QByteArray &data) {
		if (!buf)
			return;
		const int needed = data.size();
		if (needed > int(buf->size())) {
			buf.reset(m_rhi->newBuffer(QRhiBuffer::Dynamic,
			                           QRhiBuffer::VertexBuffer, needed));
			buf->create();
		}
		if (needed > 0) {
			rub->updateDynamicBuffer(buf.get(), 0, needed, data.constData());
		}
	};
	ensureUpdate(m_wireVbuf, wireData);
	ensureUpdate(m_exitsVbuf, exitsData);
	ensureUpdate(m_doorsVbuf, doorsData);
	if (!m_markersVbuf) {
		m_markersVbuf.reset(m_rhi->newBuffer(
		    QRhiBuffer::Dynamic, QRhiBuffer::VertexBuffer,
		    qMax(6, m_markersVertexCount) * 6 * int(sizeof(float))));
		m_markersVbuf->create();
	}
	ensureUpdate(m_markersVbuf, markersData);

	// STAGE 6: MATRICES shared with SHADERS (Y‑flip + X‑flip)
	const QSize outputSize = renderTarget()->pixelSize();

	QMatrix4x4 proj;
	proj = m_rhi->clipSpaceCorrMatrix();
	proj.perspective(float(fovy),
	                 outputSize.height()
	                     ? (outputSize.width() / float(outputSize.height()))
	                     : 1.0f,
	                 1.0f, 10000.0f);

	QMatrix4x4 view;
	QVector3D right{ 1, 0, 0 };
	QVector3D up{ 0, 0, 1 };
	if (m_fieldData->hasCaFile()
	    && camID < m_fieldData->getCaFile()->cameraCount()) {
		const Camera &cam = m_fieldData->getCaFile()->camera(camID);
		QVector3D eye(cam.camera_position[0], cam.camera_position[1],
		              cam.camera_position[2]);
		QVector3D rightAxis(cam.camera_axis[0].x, cam.camera_axis[0].y,
		                    cam.camera_axis[0].z);
		QVector3D upAxis(cam.camera_axis[1].x, cam.camera_axis[1].y,
		                 cam.camera_axis[1].z);
		QVector3D fwd(cam.camera_axis[2].x, cam.camera_axis[2].y,
		              cam.camera_axis[2].z);
		eye += m_xTrans * rightAxis + m_yTrans * upAxis;
		QVector3D center = eye + fwd;
		view.lookAt(eye, center, upAxis);
	} else {
		QVector3D eye(0.0f + m_xTrans, 0.0f + m_yTrans, 500.0f);
		QVector3D center(m_xTrans, m_yTrans, 0.0f);
		view.lookAt(eye, center, QVector3D(0, 1, 0));
	}

	QMatrix4x4 model;
	model.setToIdentity();
	model.scale(-1.0f, -1.0f, 1.0f); // X,Y flip
	model.rotate(m_zRot, 0.0f, 0.0f, 1.0f);
	model.rotate(m_yRot, 0.0f, 1.0f, 0.0f);
	model.rotate(m_xRot, 1.0f, 0.0f, 0.0f);

	const QMatrix4x4 mv = view * model;
	bool okInv = false;
	const QMatrix4x4 invMV = mv.inverted(&okInv);
	if (okInv) {
		right = invMV.column(0).toVector3D();
		up = invMV.column(1).toVector3D();
	}

	const QMatrix4x4 mvp = proj * view * model;
	rub->updateDynamicBuffer(m_ubuf.get(), 0, 64, mvp.constData());

	// STAGE 7: COMMAND BUFFER START PASS + DRAWS
	const QColor clearColor = m_backgroundVisible
	                              ? Qt::black
	                              : QColor::fromRgbF(0.2f, 0.2f, 0.2f, 1.0f);
	cb->beginPass(renderTarget(), clearColor, { 1.0f, 0 }, rub);

	// Background first (depth off in bg pipeline)
	if (m_backgroundVisible && m_bgVbuf && m_bgPipeline && m_bgSrb
	    && m_bgTexture) {
		cb->setGraphicsPipeline(m_bgPipeline.get());
		cb->setViewport(
		    QRhiViewport(0, 0, outputSize.width(), outputSize.height()));
		cb->setShaderResources(m_bgSrb.get());
		const QRhiCommandBuffer::VertexInput bgBinding(m_bgVbuf.get(), 0);
		cb->setVertexInput(0, 1, &bgBinding);
		cb->draw(6);
	}

	// Lines
	if (m_linePipeline && m_srb) {
		cb->setGraphicsPipeline(m_linePipeline.get());
		cb->setViewport(
		    QRhiViewport(0, 0, outputSize.width(), outputSize.height()));
		cb->setShaderResources(m_srb.get());

		// Wiremesh
		if (m_wireVbuf && m_wireVertexCount > 0) {
			const QRhiCommandBuffer::VertexInput binding(m_wireVbuf.get(), 0);
			cb->setVertexInput(0, 1, &binding);
			cb->draw(m_wireVertexCount);
		}
		// Exits
		if (m_exitsVbuf && m_exitsVertexCount > 0) {
			const QRhiCommandBuffer::VertexInput binding(m_exitsVbuf.get(), 0);
			cb->setVertexInput(0, 1, &binding);
			cb->draw(m_exitsVertexCount);
		}
		// Doors
		if (m_doorsVbuf && m_doorsVertexCount > 0) {
			const QRhiCommandBuffer::VertexInput binding(m_doorsVbuf.get(), 0);
			cb->setVertexInput(0, 1, &binding);
			cb->draw(m_doorsVertexCount);
		}
	}

	// Markers of selection (squares)
	if (m_markersVbuf && m_pipeline && m_srb && m_markersVertexCount > 0) {
		cb->setGraphicsPipeline(m_pipeline.get());
		cb->setShaderResources(m_srb.get());
		const QRhiCommandBuffer::VertexInput markersBinding(m_markersVbuf.get(),
		                                                    0);
		cb->setVertexInput(0, 1, &markersBinding);
		cb->draw(m_markersVertexCount);
	}

	cb->endPass();
	// We are done, return control :)
}

void WalkmeshRenderWidget::releaseResources()
{
	this->resetResources();
}

void WalkmeshRenderWidget::resetResources()
{
	m_pipeline.reset();
	m_linePipeline.reset();
	m_bgPipeline.reset();

	m_srb.reset();
	m_bgSrb.reset();

	m_ubuf.reset();

	m_wireVbuf.reset();
	m_exitsVbuf.reset();
	m_doorsVbuf.reset();
	m_markersVbuf.reset();
	m_bgVbuf.reset();

	m_bgTexture.reset();
	m_bgSampler.reset();

	m_rhi = nullptr;
}
