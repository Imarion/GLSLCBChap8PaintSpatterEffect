#include "PaintSpatter.h"

#include <QtGlobal>

#include <QDebug>
#include <QFile>
#include <QImage>
#include <QTime>

#include <QVector2D>
#include <QVector3D>
#include <QMatrix4x4>

#include <gtc/noise.hpp>

#include <cmath>
#include <cstring>

MyWindow::~MyWindow()
{
    if (mProgram != 0) delete mProgram;
}

MyWindow::MyWindow()
    : mProgram(0), currentTimeMs(0), currentTimeS(0), tPrev(0), angle(0.957283f)
{
    setSurfaceType(QWindow::OpenGLSurface);
    setFlags(Qt::Window | Qt::WindowSystemMenuHint | Qt::WindowTitleHint | Qt::WindowMinMaxButtonsHint | Qt::WindowCloseButtonHint);

    QSurfaceFormat format;
    format.setDepthBufferSize(24);
    format.setMajorVersion(4);
    format.setMinorVersion(3);
    format.setSamples(4);
    format.setProfile(QSurfaceFormat::CoreProfile);
    setFormat(format);
    create();

    resize(800, 600);

    mContext = new QOpenGLContext(this);
    mContext->setFormat(format);
    mContext->create();

    mContext->makeCurrent( this );

    mFuncs = mContext->versionFunctions<QOpenGLFunctions_4_3_Core>();
    if ( !mFuncs )
    {
        qWarning( "Could not obtain OpenGL versions object" );
        exit( 1 );
    }
    if (mFuncs->initializeOpenGLFunctions() == GL_FALSE)
    {
        qWarning( "Could not initialize core open GL functions" );
        exit( 1 );
    }

    initializeOpenGLFunctions();

    QTimer *repaintTimer = new QTimer(this);
    connect(repaintTimer, &QTimer::timeout, this, &MyWindow::render);
    repaintTimer->start(1000/60);

    QTimer *elapsedTimer = new QTimer(this);
    connect(elapsedTimer, &QTimer::timeout, this, &MyWindow::modCurTime);
    elapsedTimer->start(1);       
}

void MyWindow::modCurTime()
{
    currentTimeMs++;
    currentTimeS=currentTimeMs/1000.0f;
}

void MyWindow::initialize()
{
    CreateVertexBuffer();
    initShaders();
    initMatrices();
    GenerateTexture(32.0f, 0.5f, 128, 128, false);
    mRotationMatrixLocation = mProgram->uniformLocation("RotationMatrix");

    glFrontFace(GL_CCW);
    glEnable(GL_DEPTH_TEST);
}

void MyWindow::CreateVertexBuffer()
{
    // *** Teapot
    mFuncs->glGenVertexArrays(1, &mVAOTeapot);
    mFuncs->glBindVertexArray(mVAOTeapot);

    QMatrix4x4 transform;
    //transform.translate(QVector3D(0.0f, 1.5f, 0.25f));
    mTeapot = new Teapot(14, transform);

    // Create and populate the buffer objects
    unsigned int TeapotHandles[4];
    glGenBuffers(4, TeapotHandles);

    glBindBuffer(GL_ARRAY_BUFFER, TeapotHandles[0]);
    glBufferData(GL_ARRAY_BUFFER, (3 * mTeapot->getnVerts()) * sizeof(float), mTeapot->getv(), GL_STATIC_DRAW);

    glBindBuffer(GL_ARRAY_BUFFER, TeapotHandles[1]);
    glBufferData(GL_ARRAY_BUFFER, (3 * mTeapot->getnVerts()) * sizeof(float), mTeapot->getn(), GL_STATIC_DRAW);

    glBindBuffer(GL_ARRAY_BUFFER, TeapotHandles[2]);
    glBufferData(GL_ARRAY_BUFFER, (2 * mTeapot->getnVerts()) * sizeof(float), mTeapot->gettc(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, TeapotHandles[3]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, 6 * mTeapot->getnFaces() * sizeof(unsigned int), mTeapot->getelems(), GL_STATIC_DRAW);

    // Setup the VAO
    // Vertex positions
    mFuncs->glBindVertexBuffer(0, TeapotHandles[0], 0, sizeof(GLfloat) * 3);
    mFuncs->glVertexAttribFormat(0, 3, GL_FLOAT, GL_FALSE, 0);
    mFuncs->glVertexAttribBinding(0, 0);

    // Vertex normals
    mFuncs->glBindVertexBuffer(1, TeapotHandles[1], 0, sizeof(GLfloat) * 3);
    mFuncs->glVertexAttribFormat(1, 3, GL_FLOAT, GL_FALSE, 0);
    mFuncs->glVertexAttribBinding(1, 1);

    // Vertex texure coordinates
    mFuncs->glBindVertexBuffer(2, TeapotHandles[2], 0, sizeof(GLfloat) * 2);
    mFuncs->glVertexAttribFormat(2, 2, GL_FLOAT, GL_FALSE, 0);
    mFuncs->glVertexAttribBinding(2, 2);

    // Indices
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, TeapotHandles[3]);

    mFuncs->glBindVertexArray(0);

    // *** Plane
    mFuncs->glGenVertexArrays(1, &mVAOPlane);
    mFuncs->glBindVertexArray(mVAOPlane);

    mPlane = new VBOPlane(50.0f, 50.0f, 1.0, 1.0);

    // Create and populate the buffer objects
    unsigned int PlaneHandles[4];
    glGenBuffers(4, PlaneHandles);

    glBindBuffer(GL_ARRAY_BUFFER, PlaneHandles[0]);
    glBufferData(GL_ARRAY_BUFFER, (3 * mPlane->getnVerts()) * sizeof(float), mPlane->getv(), GL_STATIC_DRAW);

    glBindBuffer(GL_ARRAY_BUFFER, PlaneHandles[1]);
    glBufferData(GL_ARRAY_BUFFER, (3 * mPlane->getnVerts()) * sizeof(float), mPlane->getn(), GL_STATIC_DRAW);

    glBindBuffer(GL_ARRAY_BUFFER, PlaneHandles[2]);
    glBufferData(GL_ARRAY_BUFFER, (2 * mPlane->getnVerts()) * sizeof(float), mPlane->gettc(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, PlaneHandles[3]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, 6 * mPlane->getnFaces() * sizeof(unsigned int), mPlane->getelems(), GL_STATIC_DRAW);

    // Setup the VAO
    // Vertex positions
    mFuncs->glBindVertexBuffer(0, PlaneHandles[0], 0, sizeof(GLfloat) * 3);
    mFuncs->glVertexAttribFormat(0, 3, GL_FLOAT, GL_FALSE, 0);
    mFuncs->glVertexAttribBinding(0, 0);

    // Vertex normals
    mFuncs->glBindVertexBuffer(1, PlaneHandles[1], 0, sizeof(GLfloat) * 3);
    mFuncs->glVertexAttribFormat(1, 3, GL_FLOAT, GL_FALSE, 0);
    mFuncs->glVertexAttribBinding(1, 1);

    // Vertex texure coordinates
    mFuncs->glBindVertexBuffer(2, PlaneHandles[2], 0, sizeof(GLfloat) * 2);
    mFuncs->glVertexAttribFormat(2, 2, GL_FLOAT, GL_FALSE, 0);
    mFuncs->glVertexAttribBinding(2, 2);

    // Indices
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, PlaneHandles[3]);

    mFuncs->glBindVertexArray(0);

}

void MyWindow::initMatrices()
{
    ModelMatrixPlane.translate(0.0f, -0.45f, 0.0f);

    ModelMatrixTeapot.rotate( -90.0f, QVector3D(1.0f,0.0f,0.0f));

    ViewMatrix.lookAt(QVector3D(0.0f, 3.0f, 5.0f), QVector3D(0.0f,0.75f,0.0f), QVector3D(0.0f,1.0f,0.0f));
}

void MyWindow::resizeEvent(QResizeEvent *)
{
    mUpdateSize = true;

    ProjectionMatrix.setToIdentity();
    ProjectionMatrix.perspective(70.0f, (float)this->width()/(float)this->height(), 0.3f, 100.0f);
}

void MyWindow::render()
{
    if(!isVisible() || !isExposed())
        return;

    if (!mContext->makeCurrent(this))
        return;

    static bool initialized = false;
    if (!initialized) {
        initialize();
        initialized = true;
    }

    if (mUpdateSize) {
        glViewport(0, 0, size().width(), size().height());
        mUpdateSize = false;
    }

    float deltaT = currentTimeS - tPrev;
    if(tPrev == 0.0f) deltaT = 0.0f;
    tPrev = currentTimeS;
    angle += 0.25f * deltaT;
    if (angle > TwoPI) angle -= TwoPI;

    static float EvolvingVal = 0.0005f;
    //EvolvingVal += 0.1f;

    if (animate == true)
    {
        Threshold += EvolvingVal;

        if ((Threshold <= 0.55f) && (splatting == true))
        {
            EvolvingVal    = 0.0005f;
            splatting = false;
        }
        else if ((Threshold >= 0.80f) && (splatting == false))
        {
            EvolvingVal    = -0.0005f;
            splatting = true;
        }
    }

    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    //QMatrix4x4 RotationMatrix;
    //RotationMatrix.rotate(EvolvingVal, QVector3D(0.1f, 0.0f, 0.1f));
    //ModelMatrix.rotate(0.3f, QVector3D(0.1f, 0.0f, 0.1f));
    //QVector4D worldLight = QVector4D(10.0f * cos(angle), 3.0f, 10.0f * sin(angle), 1.0f);
    QVector4D worldLight = QVector4D(4.0f, 2.0f, 0.0f, -1.0f);

    // *** Draw teapot
    mFuncs->glBindVertexArray(mVAOTeapot);

    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glEnableVertexAttribArray(2);

    mProgram->bind();
    {
        mProgram->setUniformValue("Light.Position", ViewMatrix * worldLight );
        mProgram->setUniformValue("Light.Intensity", QVector3D(0.9f, 0.9f, 0.9f));

        mProgram->setUniformValue("Material.Kd", 0.7f, 0.5f, 0.3f);
        mProgram->setUniformValue("Material.Ks", 0.95f, 0.95f, 0.95f);
        mProgram->setUniformValue("Material.Ka", 0.1f, 0.1f, 0.1f);
        mProgram->setUniformValue("Material.Shininess", 100.0f);

        QMatrix4x4 mv1 = ViewMatrix * ModelMatrixTeapot;
        mProgram->setUniformValue("ModelViewMatrix", mv1);
        mProgram->setUniformValue("NormalMatrix", mv1.normalMatrix());
        mProgram->setUniformValue("MVP", ProjectionMatrix * mv1);

        mProgram->setUniformValue("Tex1", 0);

        mProgram->setUniformValue("Threshold",  Threshold);

        glDrawElements(GL_TRIANGLES, 6 * mTeapot->getnFaces(), GL_UNSIGNED_INT, ((GLubyte *)NULL + (0)));

        glDisableVertexAttribArray(0);
        glDisableVertexAttribArray(1);
        glDisableVertexAttribArray(2);
    }
    mProgram->release();

    // *** Draw plane
    mFuncs->glBindVertexArray(mVAOPlane);

    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glEnableVertexAttribArray(2);

    mProgram->bind();
    {
        mProgram->setUniformValue("Light.Position", ViewMatrix * worldLight );
        mProgram->setUniformValue("Light.Intensity", QVector3D(0.9f, 0.9f, 0.9f));

        mProgram->setUniformValue("Material.Kd", 0.7f, 0.7f, 0.7f);
        mProgram->setUniformValue("Material.Ks", 0.9f, 0.9f, 0.9f);
        mProgram->setUniformValue("Material.Ka", 0.1f, 0.1f, 0.1f);
        mProgram->setUniformValue("Material.Shininess", 180.0f);

        QMatrix4x4 mv1 = ViewMatrix * ModelMatrixPlane;
        mProgram->setUniformValue("ModelViewMatrix", mv1);
        mProgram->setUniformValue("NormalMatrix", mv1.normalMatrix());
        mProgram->setUniformValue("MVP", ProjectionMatrix * mv1);

        mProgram->setUniformValue("Threshold",  100.0f);

        glDrawElements(GL_TRIANGLES, 6 * mPlane->getnFaces(), GL_UNSIGNED_INT, ((GLubyte *)NULL + (0)));

        glDisableVertexAttribArray(0);
        glDisableVertexAttribArray(1);
        glDisableVertexAttribArray(2);
    }
    mProgram->release();


//    glEnableVertexAttribArray(0);
//    glEnableVertexAttribArray(1);

    mContext->swapBuffers(this);
}

void MyWindow::initShaders()
{
    QOpenGLShader vShader(QOpenGLShader::Vertex);
    QOpenGLShader fShader(QOpenGLShader::Fragment);    
    QFile         shaderFile;
    QByteArray    shaderSource;

    //Simple ADS
    shaderFile.setFileName(":/vshader.txt");
    shaderFile.open(QIODevice::ReadOnly);
    shaderSource = shaderFile.readAll();
    shaderFile.close();
    qDebug() << "vertex compile: " << vShader.compileSourceCode(shaderSource);

    shaderFile.setFileName(":/fshader.txt");
    shaderFile.open(QIODevice::ReadOnly);
    shaderSource = shaderFile.readAll();
    shaderFile.close();
    qDebug() << "frag   compile: " << fShader.compileSourceCode(shaderSource);

    mProgram = new (QOpenGLShaderProgram);
    mProgram->addShader(&vShader);
    mProgram->addShader(&fShader);
    qDebug() << "shader link: " << mProgram->link();
}

void MyWindow::GenerateTexture(float baseFreq, float persistence, int w, int h, bool periodic)
{
    int width = w;
    int height = h;

    printf("Generating noise texture...");

    GLubyte *data = new GLubyte[ width * height * 4 ];

    float xFactor = 1.0f / (width - 1);
    float yFactor = 1.0f / (height - 1);

    for( int row = 0; row < height; row++ ) {
        for( int col = 0 ; col < width; col++ ) {
            float x = xFactor * col;
            float y = yFactor * row;
            float sum = 0.0f;
            float freq = baseFreq;
            float persist = persistence;
            for( int oct = 0; oct < 4; oct++ ) {
                glm::vec2 p(x * freq, y * freq);

                float val = 0.0f;
                if (periodic) {
                  val = glm::perlin(p, glm::vec2(freq)) * persist;
                } else {
                  val = glm::perlin(p) * persist;
                }

                sum += val;

                float result = (sum + 1.0f) / 2.0f;

                // Clamp strictly between 0 and 1
                result = result > 1.0f ? 1.0f : result;
                result = result < 0.0f ? 0.0f : result;

                // Store in texture
                data[((row * width + col) * 4) + oct] = (GLubyte) ( result * 255.0f );
                freq *= 2.0f;
                persist *= persistence;
            }
        }
    }

    glActiveTexture(GL_TEXTURE0);

    GLuint TexObject;
    glGenTextures(1, &TexObject);
    glBindTexture(GL_TEXTURE_2D, TexObject);
    mFuncs->glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, width, height);
    mFuncs->glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, data);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,     GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,     GL_REPEAT);

    delete [] data;
}

void MyWindow::PrepareTexture(GLenum TextureTarget, const QString& FileName, GLuint& TexObject, bool flip)
{
    QImage TexImg;

    if (!TexImg.load(FileName)) qDebug() << "Erreur chargement texture";
    if (flip==true) TexImg=TexImg.mirrored();

    glGenTextures(1, &TexObject);
    glBindTexture(TextureTarget, TexObject);
    glTexImage2D(TextureTarget, 0, GL_RGB, TexImg.width(), TexImg.height(), 0, GL_BGRA, GL_UNSIGNED_BYTE, TexImg.bits());
    glTexParameterf(TextureTarget, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameterf(TextureTarget, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
}

void MyWindow::keyPressEvent(QKeyEvent *keyEvent)
{
    switch(keyEvent->key())
    {
        case Qt::Key_P:
            break;
        case Qt::Key_Up:
            break;
        case Qt::Key_Down:
            break;
        case Qt::Key_Left:
            break;
        case Qt::Key_Right:
            break;
        case Qt::Key_Delete:
            break;
        case Qt::Key_PageDown:
            break;
        case Qt::Key_Home:
            break;
        case Qt::Key_Z:
            break;
        case Qt::Key_Q:
            break;
        case Qt::Key_S:
            break;
        case Qt::Key_D:
            animate = !animate;
            break;
        case Qt::Key_A:
            break;
        case Qt::Key_E:
            break;
        default:
            break;
    }
}

void MyWindow::printMatrix(const QMatrix4x4& mat)
{
    const float *locMat = mat.transposed().constData();

    for (int i=0; i<4; i++)
    {
        qDebug() << locMat[i*4] << " " << locMat[i*4+1] << " " << locMat[i*4+2] << " " << locMat[i*4+3];
    }
}
