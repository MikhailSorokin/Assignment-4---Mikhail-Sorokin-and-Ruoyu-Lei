#include "GLview.hpp"

#include <iostream>
#include <algorithm>
#include <cmath>

using namespace std;

float copysign0(float x, float y) { return (y == 0.0f) ? 0 : copysign(x,y); }
static int counter = -1;
static int ani_mtl_cont = 0;
static double zaxis = 0.00;
static int moveup_counter = 0;
static int movedown_counter = 0;
static int cycle_group_cont = -1;
static double rotate_wheel_zaxis = 0.00;

void Matrix2Quaternion(QQuaternion &Q, QMatrix4x4 &M) {
  Q.setScalar(sqrt( max( 0.0f, 1 + M(0,0)  + M(1,1) + M(2,2) ) ) / 2);
  Q.setX(sqrt( max( 0.0f, 1 + M(0,0) - M(1,1) - M(2,2) ) ) / 2);
  Q.setY(sqrt( max( 0.0f, 1 - M(0,0) + M(1,1) - M(2,2) ) ) / 2);
  Q.setZ(sqrt( max( 0.0f, 1 - M(0,0) - M(1,1) + M(2,2) ) ) / 2);
  Q.setX(copysign0( Q.x(), M(2,1) - M(1,2) ));
  Q.setY(copysign0( Q.y(), M(0,2) - M(2,0) ));
  Q.setZ(copysign0( Q.z(), M(1,0) - M(0,1) )) ;
}


// GLView constructor. DO NOT MODIFY.
GLview::GLview(QWidget *parent)  : QOpenGLWidget(parent) {
  startTimer(20, Qt::PreciseTimer);
  elapsed_time.start(); elapsed_time.invalidate();  
}  

// Load object file. DO NOT MODIFY.
bool GLview::LoadOBJFile(const QString file, const QString path) {
  makeCurrent();  // Need to grab OpenGL context
  Mesh *newmesh = new Mesh;
  if(!newmesh->load_obj(file, path)) {
    delete newmesh;
    return false;
  }
  if(mesh != NULL) {  delete mesh; }
  mesh = newmesh;
  mesh->storeVBO_groups();

  doneCurrent(); // Release OpenGL context.
  return true;
}

// Set default GL parameters. 
void GLview::initializeGL() {
  initializeOpenGLFunctions();
  vao.create(); if (vao.isCreated()) vao.bind();

  glClearColor( 0.15, 0.15, 0.15, 1.0f );   // Set the clear color to black
  glEnable(GL_DEPTH_TEST);    // Enable depth buffer

  // Prepare a complete shader program...
  if ( !prepareShaderProgram(shaders,  ":/texture.vsh", ":/texture.fsh" ) ) return;

  // Enable buffer names in shader for position, normal and texture coordinates.
  shaders.bind();
  shaders.enableAttributeArray( "VertexPosition" );
  shaders.enableAttributeArray( "VertexNormal" );      
  shaders.enableAttributeArray( "VertexTexCoord" );
  
  // Set default lighting parameters. 
  LightDirection = QVector3D(1,1,0.75);
  LightIntensity = QVector3D(1,1,1);

  // Initialize default camera parameters
  yfov = 55;
  neardist = 1; fardist = 1000;
  eye = QVector3D(-3,3,3); lookCenter = QVector3D(0,0,0); lookUp = QVector3D(0,0,1);
  QMatrix4x4 view; view.lookAt(eye, lookCenter, lookUp);
  Matrix2Quaternion(camrot, view);
}

// Set the viewport to window dimensions. DO NOT MODIFY.
void GLview::resizeGL( int w, int h ) {  glViewport( 0, 0, w, qMax( h, 1 ) ); }

void GLview::paintGL() {
  if(mesh == NULL) return; // Nothing to draw.

  if (animate_mtl_flag && ani_mtl_cont < 9) {
    if (ani_mtl_cont == 0) {
      ani_mtl_cont++;
      moveup_counter = 0;
      movedown_counter = 0;
    } else {
      if (moveup_counter < 120){
        zaxis += 0.01;
        moveup_counter++;
      } else if (movedown_counter < 120){
        zaxis -= 0.01;
        movedown_counter++;
      } else {
        ani_mtl_cont++;
        moveup_counter = 0;
        movedown_counter = 0;
      }
    }
  }

  if (rotate_wheels_flag) {
    rotate_wheel_zaxis += 300.f * 0.01f;
  } else if (swerve_wheels_flag) {
      if (move_left) {
        if (moveup_counter < 45){
          zaxis += 0.01 * 100.f;
          moveup_counter++;
        } else if (movedown_counter < 45){
          zaxis -= 0.01 * 100.f;
          movedown_counter++;
        } else {
          moveup_counter = 0;
          movedown_counter = 0;
          move_left = false;
        }
      } else {
        if (moveup_counter < 45){
          zaxis -= 0.01 * 100.f;
          moveup_counter++;
        } else if (movedown_counter < 45){
          zaxis += 0.01 * 100.f;
          movedown_counter++;
        } else {
          moveup_counter = 0;
          movedown_counter = 0;
          move_left = true;
        }
      }
  }

/*
  if (cycle_group_flag && cycle_group_cont > 67) {
    cycle_group_flag = false;
    cycle_group_cont = -1;
  }
*/
  // Clear the frame buffer with the current clearing color and clear depth buffer.
  glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

  shaders.bind();
  for(long group_idx = 0; group_idx < (long)mesh->groups.size(); group_idx++) {
    vector<Mesh_Material> &materials = mesh->groups[group_idx].materials;

    if (cycle_group_flag && cycle_group_cont != group_idx) continue;

    for(long mtl_idx = 0; mtl_idx < (long)materials.size(); mtl_idx++) {
      if (cycle_mtl_flag && (mtl_idx != counter)) continue;
      if (materials[mtl_idx].n_triangles == 0) continue;
      
      QMatrix4x4 model;

      if (animate_mtl_flag && ani_mtl_cont == mtl_idx) {
        QVector3D moveup(0,0,zaxis);
        model.translate(moveup);
        
      } else if (rotate_wheels_flag && materials[mtl_idx].name == "wheel") {
          // cout << "(" << mesh->groups[group_idx].center.x() << ", " << mesh->groups[group_idx].center.y() << ", " << mesh->groups[group_idx].center.z() << ")" << endl;
          model.translate(mesh->groups[group_idx].center);
          model.rotate(rotate_wheel_zaxis, 1.0f, 0.0f, 0.0f);
          model.translate(-mesh->groups[group_idx].center);
      } else if (swerve_wheels_flag &&
                 (materials[mtl_idx].name == "tyre" || materials[mtl_idx].name == "wheel" || materials[mtl_idx].name == "tread")) {
          
          if (materials[mtl_idx].name == "wheel") {
            if (group_idx == 17 || group_idx == 18) {
              // front wheels
              rotate_wheel_zaxis += 300.f * 0.01f;
              model.translate(mesh->groups[group_idx].center);
              model.rotate(zaxis, lookUp);
              model.rotate(rotate_wheel_zaxis, 1.0f, 0.0f, 0.0f);
              model.translate(-mesh->groups[group_idx].center);
            } else {
              // rear wheels
              model.translate(mesh->groups[group_idx].center);
              model.rotate(rotate_wheel_zaxis, 1.0f, 0.0f, 0.0f);
              model.translate(-mesh->groups[group_idx].center);
            }
          } else if (group_idx == 42 || group_idx == 43 || group_idx == 44 || group_idx == 45 || group_idx == 46 || group_idx == 47){
              
              model.translate(mesh->groups[group_idx].center);
              model.rotate(zaxis, lookUp);
              model.translate(-mesh->groups[group_idx].center);
              
          }
      } else {
          model.translate(mesh->model_translate);
          model.rotate(mesh->model_rotation);
      }

      model.scale(mesh->model_sx, mesh->model_sy, mesh->model_sz);      
      QMatrix4x4 view, projection;

      view.rotate(camrot); view.translate(-eye);
      projection.perspective(yfov, (float)width() / (float)height(), neardist, fardist);
    
      QMatrix4x4 model_view = view * model;
      QMatrix3x3 normal_matrix = model_view.normalMatrix();
      QMatrix4x4 MVP = projection * model_view;

      shaders.setUniformValue("ModelViewMatrix", model_view);
      shaders.setUniformValue("NormalMatrix", normal_matrix);
      shaders.setUniformValue("MVP", MVP);
      shaders.setUniformValue("LightIntensity", LightIntensity);
      // NOTE: Must multiply light position by view matrix!!!!!
      shaders.setUniformValue("LightDirection", view * QVector4D(LightDirection, 0));

      // Bind geometry buffers for current group and material. 
      materials[mtl_idx].vertexBuffer->bind();
      shaders.setAttributeBuffer( "VertexPosition", GL_FLOAT, 0, 3 );
      materials[mtl_idx].normalBuffer->bind();
      shaders.setAttributeBuffer( "VertexNormal", GL_FLOAT, 0, 3 );
      materials[mtl_idx].texCoordBuffer->bind();
      shaders.setAttributeBuffer( "VertexTexCoord", GL_FLOAT, 0, 2 );

      // Update shading parameters for current material.
      shaders.setUniformValue("Kd", materials[mtl_idx].Kd);
      shaders.setUniformValue("Ks", materials[mtl_idx].Ks);
      shaders.setUniformValue("Ka", materials[mtl_idx].Ka);
      shaders.setUniformValue("Shininess", materials[mtl_idx].Ns);
      shaders.setUniformValue("useTexture", false);
      
      // If a texture material, so use for drawing.
      if(materials[mtl_idx].map_Kd != NULL) {
	shaders.setUniformValue("useTexture", true);
	// Bind texture to texture unit 0.
	materials[mtl_idx].map_Kd->bind(0); 
	shaders.setUniformValue("Tex1", (int)0); // Update shader uniform.
      }
      // Starting at index 0, draw 3 * n_triangle verticies from current geometry buffers.
      glDrawArrays( GL_TRIANGLES, 0, 3 * materials[mtl_idx].n_triangles );

    }
  }
  
}

void GLview::keyPressGL(QKeyEvent* e) {
  switch ( e->key() ) {
  case Qt::Key_Escape: QCoreApplication::instance()->quit(); break;
    // Set rotate, scale, translate modes (for track pad mainly).
  case Qt::Key_R: toggleRotate();    break;
  case Qt::Key_S: toggleScale();     break;
  case Qt::Key_T: toggleTranslate();     break;
  case Qt::Key_6: cycle_material();    break;
  case Qt::Key_7: cycle_group();    break;
  default: QOpenGLWidget::keyPressEvent(e);  break;
  }
}


// Compile shaders. DO NOT MODIFY.
bool GLview::prepareShaderProgram(QOpenGLShaderProgram &prep_shader, 
				  const QString &vertex_file, const QString &fragment_file) {
  // First we load and compile the vertex shader.
  bool result = prep_shader.addShaderFromSourceFile( QOpenGLShader::Vertex, vertex_file );
  if ( !result ) qWarning() << prep_shader.log();

  // ...now the fragment shader...
  result = prep_shader.addShaderFromSourceFile( QOpenGLShader::Fragment, fragment_file );
  if ( !result ) qWarning() << prep_shader.log();

  // ...and finally we link them to resolve any references.
  result = prep_shader.link();
  if ( !result ) {   qWarning() << "Could not link shader program:" << prep_shader.log(); exit(1);   }
  return result;
}

// Store mouse press position. DO NOT MODIFY
void GLview::mousePressEvent(QMouseEvent *event) {
  if (event->button()==Qt::RightButton) {
    ShowContextMenu(event);
  }
  if (event->button()==Qt::LeftButton) {
    if(mesh == NULL) return;
    float px = event->x(), py = event->y();
    // Translate to OpenGL coordinates (and middle of pixel).
    float x = 2.0 * (px + 0.5) / float(width())  - 1.0;
    float y = -(2.0 * (py + 0.5) / float(height()) - 1.0); 
    lastPosX = x; lastPosY = y;
    lastPosFlag = true; 
    event->accept();
  }
}

// show the right-click popup menu. DO NOT MODIFY
void GLview::ShowContextMenu(const QMouseEvent *event) {
  // You made add additional menu options for the extra credit below,
  // please use the template below to add additional menu options.
  QMenu menu;

  QAction* option0 = new QAction("Light Direction Motion", this);
  connect(option0, SIGNAL(triggered()), this, SLOT(light_motion()));

  QAction* option1 = new QAction("Animate FOV", this);
  connect(option1, SIGNAL(triggered()), this, SLOT(animate_fov()));

  QAction* option2 = new QAction("Animate Near Plane", this);
  connect(option2, SIGNAL(triggered()), this, SLOT(animate_near()));

  QAction* option3 = new QAction("Animate Far Plane", this);
  connect(option3, SIGNAL(triggered()), this, SLOT(animate_far()));

  QAction* option4 = new QAction("Animate Camera", this);
  connect(option4, SIGNAL(triggered()), this, SLOT(animate_camera()));

  QAction* option5 = new QAction("Cycle Material", this);
  connect(option5, SIGNAL(triggered()), this, SLOT(cycle_material()));

  QAction* option6 = new QAction("Animate Material", this);
  connect(option6, SIGNAL(triggered()), this, SLOT(animate_material()));

  QAction* option7 = new QAction("Cycle Group", this);
  connect(option7, SIGNAL(triggered()), this, SLOT(cycle_group()));

  QAction* option8 = new QAction("Animate Rotate Wheels", this);
  connect(option8, SIGNAL(triggered()), this, SLOT(animate_rotate_wheels()));
  
  QAction* option9 = new QAction("Animate Swerve Wheels", this);
  connect(option9, SIGNAL(triggered()), this, SLOT(animate_swerve_wheels()));
  
  menu.addAction(option0); menu.addAction(option1);
  menu.addAction(option2); menu.addAction(option3);
  menu.addAction(option4); menu.addAction(option5);
  menu.addAction(option6); menu.addAction(option7);
  menu.addAction(option8); menu.addAction(option9);  
  menu.exec(mapToGlobal(event->pos()));
}

// Update camera position on mouse movement. DO NOT MODIFY.
void GLview::mouseMoveEvent(QMouseEvent *event) {
  if(mesh == NULL) return;

  float px = event->x(), py = event->y();
  float x = 2.0 * (px + 0.5) / float(width())  - 1.0;
  float y = -(2.0 * (py + 0.5) / float(height()) - 1.0); 

  // Record a last position if none has been set.  
  if(!lastPosFlag) { lastPosX = x; lastPosY = y; lastPosFlag = true; return; }
  float dx = x - lastPosX, dy = y - lastPosY;
  lastPosX = x; lastPosY = y; // Remember mouse position.

  if (rotateFlag || (event->buttons() & Qt::LeftButton)) { // Rotate scene around a center point.   
    float theta_y = 2.0 * dy / M_PI * 180.0f;
    float theta_x = 2.0 * dx / M_PI * 180.0f;

    // Get rotation from -z to camera. Rotate the camera in the wold.
    QQuaternion revQ = camrot.conjugate();
    QQuaternion newrot = QQuaternion::fromAxisAndAngle(lookUp, theta_x);
    revQ = newrot * revQ;

    QVector3D side = revQ.rotatedVector(QVector3D(1,0,0));
    QQuaternion newrot2 = QQuaternion::fromAxisAndAngle(side, theta_y);
    revQ = newrot2 * revQ;
    revQ.normalize();

    // Go back to camera frame.
    camrot = revQ.conjugate().normalized();    

    // Update camera position.
    eye = newrot.rotatedVector(eye - lookCenter) + lookCenter;
    eye = newrot2.rotatedVector(eye - lookCenter) + lookCenter;
  } 

  if (scaleFlag || (event->buttons() & Qt::MidButton)) { // Scale the scene.
    float factor = dx + dy;
    factor = exp(2.0 * factor);
    factor = (factor - 1.0) / factor;
    QVector3D translation = (lookCenter - eye) * factor; 
    eye += translation;
  }
  if (translateFlag || (event->buttons() & Qt::RightButton)) { // Translate the scene.
    // Get camera side and up vectors.
    QQuaternion revQ = camrot.conjugate().normalized();
    QVector3D side = revQ.rotatedVector(QVector3D(1,0,0));
    QVector3D upVector = revQ.rotatedVector(QVector3D(0,1,0));

    // Move camera and look center in direction of the side and up vectors of camera.
    float length = lookCenter.distanceToPoint(eye) * tanf(yfov * M_PI / 180.0f);
    QVector3D translation = -((side * (length * dx)) + (upVector * (length * dy) ));
    eye += translation;
    lookCenter += translation;
  }
  event->accept();
}


void GLview::updateGLview(float dt) {
  if(lightMotionFlag) {
    // Rotate the light direction about the up axis.
    QQuaternion q1 = QQuaternion::fromAxisAndAngle(lookUp, dt * 30);
    LightDirection = q1.rotatedVector(LightDirection);
  }
	
 /********** Camera Parameter Animations (implement all) *********/
  if (FOVFlag) {
    //Animate the FOV Plane so that it goes in between 20 and 100 degrees
    if (yfov <= 100 && incrementAngle) {
        yfov += dt * 3;
    } else if (!incrementAngle && yfov >= 20.f) {
        yfov -= dt * 3;
    } else {
        incrementAngle = !incrementAngle;
    }
    //TODO: Smoothly lerp between angles of 20 and 100 degrees using some sort of time step interval. 
  }
	
  if (animationFarFlag) {
    //Animate the Far Plane so that it goes in between 5 and 50
      if (fardist <= 50.f && incrementAngle) {
          cout << "Value: " << fardist << endl;
          fardist += dt * 30;
      } else if (!incrementAngle && fardist >= 5.f) {
          cout << "Value: " << fardist << endl;
          fardist -= dt * 30;
      } else {
          cout << "Value: " << fardist << endl;
          incrementAngle = !incrementAngle;
      }
	  
    //TODO: Smoothly lerp between far value of 500 and 1000 using some sort of time step interval. 
  }
	
  if (animationNearFlag) {
    //Animate the Near Plane so that it goes in between 1 and 50
      if (neardist <= 50.f && incrementAngle) {
          cout << "Value: " << neardist << endl;
          neardist += dt * 30;
      } else if (!incrementAngle && neardist > 1.f) {
          cout << "Value: " << neardist << endl;
          neardist -= dt * 30;
      } else {
          cout << "Value: " << neardist << endl;
          incrementAngle = !incrementAngle;
      }

    //TODO: Smoothly lerp between near value of 1 and 500 using some sort of time step interval. 
  }
	
  if (animateCameraFlag) {
    //Animate the camera position about the look center in the look up direction.
    QQuaternion revQ = camrot.conjugate();

    QQuaternion newrot = QQuaternion::fromAxisAndAngle(lookUp, 25.0 * dt);
      revQ = newrot * revQ;
      revQ.normalize();

      // Go back to camera frame.
      camrot = revQ.conjugate().normalized();

      // Update camera position.
      eye = newrot.rotatedVector(eye - lookCenter) + lookCenter;
  }
}


// Update according to timer. Keep time step fixed. DO NOT MODIFY.
void GLview::timerEvent(QTimerEvent *) {
  if(!elapsed_time.isValid()) {   elapsed_time.restart(); return;  } // Skip first update.
  qint64 nanoSec = elapsed_time.nsecsElapsed();
  elapsed_time.restart();  

  double dt = 0.01; // dt is the animation time step
  double frameTime = double(nanoSec) * 1e-9;

  timeAccumulator += frameTime;
  while ( timeAccumulator >= dt ) {
    // Keep the animation time step fixed, catching up if
    // drawing takes too long. 
    updateGLview(dt);  totalTime += dt;  timeAccumulator -= dt;
  }

  update();  
}

void GLview::light_motion() {
  if(mesh == NULL) return;  lightMotionFlag = !lightMotionFlag; 
}


void GLview::animate_fov() {
  cout << "implement animate_fov()" << endl;
  yfov = 55;
  if (mesh == NULL) return; FOVFlag = !FOVFlag;
}

void GLview::animate_near() {
  if (mesh == NULL) return; animationNearFlag = !animationNearFlag;
}

void GLview::animate_far() {
  fardist = 50.f;
  cout << "implement animate_far()" << endl;
  if (mesh == NULL) return; animationFarFlag = !animationFarFlag;
}

void GLview::animate_camera() {
  cout << "implement animate_camera()" << endl;
  if (mesh == NULL) return; animateCameraFlag  = !animateCameraFlag;
}

void GLview::cycle_material() {
  cout << "implement cycle_material()" << endl;
  if (mesh == NULL) return;

    animate_mtl_flag = false;
    cycle_group_flag = false;

  vector<string> materials = {"default. Press 6 as a shortcut for this function.","tyre","body","generic","wheel","glow","glass","tread"};
  cycle_mtl_flag = true;

  if (counter < 7) {
    counter = (counter + 1);
  } else {
    counter = 0;
    cycle_mtl_flag = false;
  }

  if (cycle_mtl_flag){
  QString material_name_text = QString::fromStdString("Material name is " + materials[counter]);
  QMessageBox::information(this, "Material Name", material_name_text);
  }
}

void GLview::animate_material() {
  cout << "implement animate_material()" << endl;
  if (mesh == NULL) return;

  cycle_mtl_flag = false;
  cycle_group_flag = false;

  zaxis = 0.00;
  moveup_counter = 0;
  movedown_counter = 0;
  ani_mtl_cont = 0;
  animate_mtl_flag = true;
  
}

void GLview::cycle_group() {
  if (mesh == NULL) return;

  cycle_mtl_flag = false;
    animate_mtl_flag = false;

  // cout << "To save time, press key 7 to call this function" << endl;

  cycle_group_flag = true;
  string obj_name = cycle_group_cont == -1 ? "default. Press 7 as a shortcut for this function." : "object__" + to_string(cycle_group_cont);
  QString group_name_text = QString::fromStdString("Group name: " + obj_name);
  QMessageBox::information(this, "Group Name", group_name_text);

  if (cycle_group_cont > 66) {
    cycle_group_cont = -1;
    cycle_group_flag = false;
  } else {
    cycle_group_cont++;
  }
}

void GLview::animate_rotate_wheels() {
  cycle_mtl_flag = false;
    animate_mtl_flag = false;
    cycle_group_flag = false;

    rotate_wheel_zaxis = 0.00;
    
    if (mesh == NULL) return; rotate_wheels_flag = !rotate_wheels_flag;
}

void GLview::animate_swerve_wheels() {
    cycle_mtl_flag = false;
    animate_mtl_flag = false;
    cycle_group_flag = false;

    zaxis = 0.00;
    moveup_counter = 0;
    movedown_counter = 0;
    
    if (mesh == NULL) return; swerve_wheels_flag = !swerve_wheels_flag;
}
