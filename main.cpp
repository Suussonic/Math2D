#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include "Complex.h"
#include "Matrix2x2.h"
#include "Transform2D.h"
#include "Fractals.h"

#include <cstdio>
#include <vector>
#include <string>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

static int g_winW = 1280, g_winH = 800;

// Convertit des coordonnees monde (wx, wy) en coordonnees NDC [-1,1] selon le centre et l echelle de la vue
static void worldToGL(double wx, double wy,
                      double cx, double cy, double scale,
                      float& ox, float& oy)
{
    ox = (float)((wx - cx) * scale);
    oy = (float)((wy - cy) * scale);
}

// Dessine un polygone (ferme ou ouvert) en coordonnees monde avec la couleur RGB donnee
static void drawPolygon(const Polygon& poly,
                        float r, float g, float b,
                        double cx, double cy, double scale,
                        bool closed=true)
{
    if (poly.empty()) return;
    glColor3f(r,g,b);
    glBegin(closed ? GL_LINE_LOOP : GL_LINE_STRIP);
    for (auto& p : poly) {
        float fx, fy;
        worldToGL(p.re, p.im, cx, cy, scale, fx, fy);
        glVertex2f(fx, fy);
    }
    glEnd();
}

// Dessine un nuage de points colores en coordonnees monde
static void drawPoints(const std::vector<std::pair<Complex,Color4f>>& pts,
                       double cx, double cy, double scale)
{
    glBegin(GL_POINTS);
    for (auto& [p,c] : pts) {
        float fx, fy;
        worldToGL(p.re, p.im, cx, cy, scale, fx, fy);
        glColor4f(c.r,c.g,c.b,c.a);
        glVertex2f(fx, fy);
    }
    glEnd();
}

// Dessine une liste de segments colores en coordonnees monde
static void drawSegments(const std::vector<Segment>& segs,
                         double cx, double cy, double scale)
{
    glBegin(GL_LINES);
    for (auto& s : segs) {
        float ax,ay,bx,by;
        worldToGL(s.A.re, s.A.im, cx, cy, scale, ax, ay);
        worldToGL(s.B.re, s.B.im, cx, cy, scale, bx, by);
        glColor4f(s.color.r, s.color.g, s.color.b, s.color.a);
        glVertex2f(ax,ay); glVertex2f(bx,by);
    }
    glEnd();
}

// Dessine une image (largeur x hauteur) de pixels dans un quad NDC
static GLuint g_texMandelbrot = 0;

// Charge (ou recharge) l image Mandelbrot dans une texture OpenGL 2D
static void uploadMandelbrotTexture(const std::vector<Color4f>& img, int w, int h)
{
    if (g_texMandelbrot == 0) glGenTextures(1, &g_texMandelbrot);
    glBindTexture(GL_TEXTURE_2D, g_texMandelbrot);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_FLOAT, img.data());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glBindTexture(GL_TEXTURE_2D, 0);
}

// Affiche la texture Mandelbrot sur un quad couvrant tout le viewport
static void drawMandelbrotTexture()
{
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, g_texMandelbrot);
    glColor3f(1,1,1);
    glBegin(GL_QUADS);
      glTexCoord2f(0,0); glVertex2f(-1,-1);
      glTexCoord2f(1,0); glVertex2f( 1,-1);
      glTexCoord2f(1,1); glVertex2f( 1, 1);
      glTexCoord2f(0,1); glVertex2f(-1, 1);
    glEnd();
    glDisable(GL_TEXTURE_2D);
}

//  Etat de l application
enum class Demo {
    Complexes = 0,
    Matrices,
    Transformations,
    Mandelbrot,
    Lightning,
    Snowflake,
    Fern,
    Brittany,
    COUNT
};
static const char* demoNames[] = {
    "Nombres complexes",
    "Matrices 2x2",
    "Transformations 2D",
    "Mandelbrot",
    "Eclairs",
    "Flocon de Koch",
    "Fougere de Barnsley",
    "Cote de Bretagne"
};

struct AppState
{
    Demo  currentDemo = Demo::Complexes;

    // Complexes
    double cre1=1, cim1=2, cre2=3, cim2=-1;

    // Transformations
    float txSlide=0.5f, tySlide=0.0f;
    float rotAngle=0.0f;
    float homoK=1.0f;
    float simARe=0.8f, simAIm=0.2f, simBRe=0.3f, simBIm=0.0f;
    int   shapeSel=0; // 0=carre, 1=triangle
    bool  showOrig=true;

    // Mandelbrot
    int   mandIter=100;
    double mandXmin=-2.5, mandXmax=1.0, mandYmin=-1.25, mandYmax=1.25;
    bool  mandDirty=true;
    int   mandW=512, mandH=512;

    // Flocon
    int   snowDepth=4;
    bool  snowDirty=true;
    std::vector<Complex> snowPts;

    // Fougere
    int   fernN=80000;
    bool  fernDirty=true;
    std::vector<std::pair<Complex,Color4f>> fernPts;

    // Foret
    bool  showForest=false;
    std::vector<std::pair<Complex,Color4f>> forestPts;

    // Eclairs
    int   lightDepth=6;
    bool  lightDirty=true;
    std::vector<Segment> lightSegs;
    unsigned lightSeed=42;

    // Bretagne
    int   coastDepth=8;
    bool  coastDirty=true;
    std::vector<Complex> coastPts;
    unsigned coastSeed=123;
};

static AppState app;

// Construit et affiche le panneau de controle ImGui (selection de demo et parametres)
static void drawDebugPanel()
{
    ImGui::SetNextWindowPos({0,0}, ImGuiCond_Always);
    ImGui::SetNextWindowSize({340,(float)g_winH}, ImGuiCond_Always);
    ImGui::Begin("Math2D Debug", nullptr,
        ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);

    // Selecteur de demo
    ImGui::Text("Demo :");
    for (int i=0; i<(int)Demo::COUNT; ++i)
    {
        if (ImGui::RadioButton(demoNames[i], (int)app.currentDemo == i))
            app.currentDemo = (Demo)i;
    }
    ImGui::Separator();

    // Parametres specifiques
    switch (app.currentDemo)
    {
    case Demo::Complexes:
    {
        ImGui::Text("z1 = %.2f + %.2fi", app.cre1, app.cim1);
        ImGui::Text("z2 = %.2f + %.2fi", app.cre2, app.cim2);
        ImGui::DragScalar("z1.re", ImGuiDataType_Double, &app.cre1, 0.05f);
        ImGui::DragScalar("z1.im", ImGuiDataType_Double, &app.cim1, 0.05f);
        ImGui::DragScalar("z2.re", ImGuiDataType_Double, &app.cre2, 0.05f);
        ImGui::DragScalar("z2.im", ImGuiDataType_Double, &app.cim2, 0.05f);
        ImGui::Separator();
        Complex z1(app.cre1,app.cim1), z2(app.cre2,app.cim2);
        ImGui::Text("z1+z2  = %s", (z1+z2).toString().c_str());
        ImGui::Text("z1-z2  = %s", (z1-z2).toString().c_str());
        ImGui::Text("z1*z2  = %s", (z1*z2).toString().c_str());
        try { ImGui::Text("z1/z2  = %s", (z1/z2).toString().c_str()); }
        catch (...) { ImGui::Text("z1/z2  = undef"); }
        ImGui::Text("conj(z1)= %s", z1.conjugate().toString().c_str());
        ImGui::Text("|z1|   = %.4f", z1.modulus());
        ImGui::Text("arg(z1)= %.4f rad", z1.argument());
        break;
    }
    case Demo::Matrices:
    {
        Complex z1(app.cre1,app.cim1), z2(app.cre2,app.cim2);
        Matrix2x2 m1 = Matrix2x2::fromComplex(z1);
        Matrix2x2 m2 = Matrix2x2::fromComplex(z2);
        Matrix2x2 prod = m1 * m2;
        Complex zp = prod.toComplex();
        Complex direct = z1 * z2;
        ImGui::DragScalar("z1.re", ImGuiDataType_Double, &app.cre1, 0.05f);
        ImGui::DragScalar("z1.im", ImGuiDataType_Double, &app.cim1, 0.05f);
        ImGui::DragScalar("z2.re", ImGuiDataType_Double, &app.cre2, 0.05f);
        ImGui::DragScalar("z2.im", ImGuiDataType_Double, &app.cim2, 0.05f);
        ImGui::Separator();
        ImGui::Text("M(z1) = |%.2f  %.2f|", m1.a, m1.b);
        ImGui::Text("        |%.2f  %.2f|", m1.c, m1.d);
        ImGui::Text("M(z2) = |%.2f  %.2f|", m2.a, m2.b);
        ImGui::Text("        |%.2f  %.2f|", m2.c, m2.d);
        ImGui::Text("M(z1)*M(z2) -> z = %s", zp.toString().c_str());
        ImGui::Text("z1*z2 direct = %s", direct.toString().c_str());
        bool ok = (std::abs(zp.re - direct.re) < 1e-9 && std::abs(zp.im - direct.im) < 1e-9);
        ImGui::TextColored(ok ? ImVec4(0,1,0,1) : ImVec4(1,0,0,1),
            ok ? "Compatible !" : "ERREUR");
        break;
    }
    case Demo::Transformations:
    {
        ImGui::Text("Figure :"); ImGui::SameLine();
        ImGui::RadioButton("Carre",    &app.shapeSel, 0); ImGui::SameLine();
        ImGui::RadioButton("Triangle", &app.shapeSel, 1);
        ImGui::Checkbox("Montrer original", &app.showOrig);
        ImGui::Separator();
        ImGui::SliderFloat("Translation X", &app.txSlide, -1.5f, 1.5f);
        ImGui::SliderFloat("Translation Y", &app.tySlide, -1.5f, 1.5f);
        ImGui::SliderFloat("Rotation (rad)", &app.rotAngle, -(float)M_PI, (float)M_PI);
        ImGui::SliderFloat("Homothetie k",   &app.homoK, 0.1f, 3.0f);
        ImGui::Separator();
        ImGui::Text("Similitude : z -> a*z + b");
        ImGui::DragFloat("a.re", &app.simARe, 0.02f, -2.f, 2.f);
        ImGui::DragFloat("a.im", &app.simAIm, 0.02f, -2.f, 2.f);
        ImGui::DragFloat("b.re", &app.simBRe, 0.02f, -2.f, 2.f);
        ImGui::DragFloat("b.im", &app.simBIm, 0.02f, -2.f, 2.f);
        break;
    }
    case Demo::Mandelbrot:
        ImGui::SliderInt("Iterations max", &app.mandIter, 10, 500);
        ImGui::DragScalar("Xmin", ImGuiDataType_Double, &app.mandXmin, 0.05);
        ImGui::DragScalar("Xmax", ImGuiDataType_Double, &app.mandXmax, 0.05);
        ImGui::DragScalar("Ymin", ImGuiDataType_Double, &app.mandYmin, 0.05);
        ImGui::DragScalar("Ymax", ImGuiDataType_Double, &app.mandYmax, 0.05);
        if (ImGui::Button("Recalculer")) app.mandDirty = true;
        break;
    case Demo::Lightning:
        ImGui::SliderInt("Profondeur", &app.lightDepth, 1, 9);
        ImGui::DragScalar("Seed", ImGuiDataType_U32, &app.lightSeed, 1.f);
        if (ImGui::Button("Regenerer")) app.lightDirty = true;
        break;
    case Demo::Snowflake:
        ImGui::SliderInt("Profondeur Koch", &app.snowDepth, 0, 7);
        if (ImGui::Button("Regenerer")) app.snowDirty = true;
        break;
    case Demo::Fern:
        ImGui::SliderInt("Nb points", &app.fernN, 1000, 200000);
        ImGui::Checkbox("Foret", &app.showForest);
        if (ImGui::Button("Regenerer")) app.fernDirty = true;
        break;
    case Demo::Brittany:
        ImGui::SliderInt("Profondeur", &app.coastDepth, 1, 12);
        ImGui::DragScalar("Seed", ImGuiDataType_U32, &app.coastSeed, 1.f);
        if (ImGui::Button("Regenerer")) app.coastDirty = true;
        break;
    default: break;
    }

    ImGui::End();
}

// Effectue le rendu OpenGL
static void renderScene()
{
    // Viewport de rendu = tout sauf le panel ImGui
    int vpX = 340, vpW = g_winW - 340;
    glViewport(vpX, 0, vpW, g_winH);
    glMatrixMode(GL_PROJECTION); glLoadIdentity();
    glMatrixMode(GL_MODELVIEW);  glLoadIdentity();
    glClear(GL_COLOR_BUFFER_BIT);

    switch (app.currentDemo)
    {
    // Complexes : visualisation vecteurs
    case Demo::Complexes:
    case Demo::Matrices:
    {
        Complex z1(app.cre1,app.cim1), z2(app.cre2,app.cim2);
        double sc = 0.3;
        // axes
        glColor3f(0.3f,0.3f,0.3f);
        glBegin(GL_LINES);
        glVertex2f(-1,0); glVertex2f(1,0);
        glVertex2f(0,-1); glVertex2f(0,1);
        glEnd();
        // z1
        glColor3f(1,0.4f,0.1f);
        glBegin(GL_LINES);
        glVertex2f(0,0);
        glVertex2f((float)(z1.re*sc),(float)(z1.im*sc));
        glEnd();
        // z2
        glColor3f(0.1f,0.8f,1);
        glBegin(GL_LINES);
        glVertex2f(0,0);
        glVertex2f((float)(z2.re*sc),(float)(z2.im*sc));
        glEnd();
        // z1*z2
        Complex prod = z1*z2;
        glColor3f(1,1,0);
        glBegin(GL_LINES);
        glVertex2f(0,0);
        glVertex2f((float)(prod.re*sc),(float)(prod.im*sc));
        glEnd();
        break;
    }

    // Transformations
    case Demo::Transformations:
    {
        double sc = 0.45;
        Polygon orig = (app.shapeSel==0)
            ? makeSquare(0,0,1)
            : makeTriangle(0,0,0.5);

        Complex t(app.txSlide, app.tySlide);
        Complex omega(0,0);
        Complex a(app.simARe, app.simAIm);
        Complex b(app.simBRe, app.simBIm);

        if (app.showOrig)
            drawPolygon(orig, 0.4f,0.4f,0.4f, 0, 0, sc);

        // Translation
        drawPolygon(applyTranslation(orig, t), 1,0.3f,0.3f, 0,0,sc);
        // Rotation
        drawPolygon(applyRotation(orig, omega, app.rotAngle), 0.3f,1,0.3f, 0,0,sc);
        // Homothetie
        drawPolygon(applyHomothetie(orig, omega, app.homoK), 0.3f,0.3f,1, 0,0,sc);
        // Similitude
        drawPolygon(applySimilitude(orig, a, b), 1,1,0.3f, 0,0,sc);
        break;
    }

    // Mandelbrot
    case Demo::Mandelbrot:
        if (app.mandDirty) {
            auto img = generateMandelbrot(app.mandW, app.mandH, app.mandIter,
                app.mandXmin, app.mandXmax, app.mandYmin, app.mandYmax);
            uploadMandelbrotTexture(img, app.mandW, app.mandH);
            app.mandDirty = false;
        }
        drawMandelbrotTexture();
        break;

    // Eclairs
    case Demo::Lightning:
        if (app.lightDirty) {
            app.lightSegs = generateLightning(app.lightDepth, app.lightSeed);
            app.lightDirty = false;
        }
        drawSegments(app.lightSegs, 0, 0, 0.85);
        break;

    // Flocon
    case Demo::Snowflake:
        if (app.snowDirty) {
            app.snowPts = generateSnowflake(app.snowDepth, 0.8);
            app.snowDirty = false;
        }
        drawPolygon(app.snowPts, 0.6f,0.9f,1.0f, 0,0, 0.9, true);
        break;

    // Fougere
    case Demo::Fern:
        if (app.fernDirty) {
            app.fernPts   = generateFern(app.fernN);
            app.forestPts = generateForest(5, app.fernN);
            app.fernDirty = false;
        }
        if (app.showForest)
            drawPoints(app.forestPts, 0, -4, 0.09);
        else
            drawPoints(app.fernPts,   0, -4, 0.13);
        break;

    // Bretagne
    case Demo::Brittany:
        if (app.coastDirty) {
            app.coastPts = generateBrittanyCoast(app.coastDepth, app.coastSeed);
            app.coastDirty = false;
        }
        {
            // Dessine la ligne de cote en bleu
            glColor3f(0.2f,0.5f,1.0f);
            glBegin(GL_LINE_STRIP);
            for (auto& p : app.coastPts)
                glVertex2f((float)p.re, (float)p.im);
            glEnd();
            // Remplissage mer
            glColor4f(0.1f,0.2f,0.6f,0.4f);
            glBegin(GL_TRIANGLE_FAN);
            glVertex2f(0,-1);
            for (auto& p : app.coastPts)
                glVertex2f((float)p.re, (float)p.im);
            glEnd();
        }
        break;

    default: break;
    }
}

//  Main
int main()
{
    if (!glfwInit()) return 1;
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);

    GLFWwindow* win = glfwCreateWindow(g_winW, g_winH, "Math2D - ESGI", nullptr, nullptr);
    if (!win) { glfwTerminate(); return 1; }
    glfwMakeContextCurrent(win);
    glfwSwapInterval(1);

    glewInit();

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(win, true);
    ImGui_ImplOpenGL3_Init("#version 120");

    glClearColor(0.1f,0.1f,0.12f,1.0f);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    while (!glfwWindowShouldClose(win))
    {
        glfwPollEvents();
        glfwGetFramebufferSize(win, &g_winW, &g_winH);

        // Viewport
        glViewport(0, 0, g_winW, g_winH);
        glClear(GL_COLOR_BUFFER_BIT);

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        drawDebugPanel();
        renderScene();

        ImGui::Render();
        glViewport(0, 0, g_winW, g_winH);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(win);
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(win);
    glfwTerminate();
    return 0;
}

