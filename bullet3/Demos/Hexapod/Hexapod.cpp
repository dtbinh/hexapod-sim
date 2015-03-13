#include "btBulletDynamicsCommon.h"
#include "GlutStuff.h"
#include "GL_ShapeDrawer.h"

#include "LinearMath/btIDebugDraw.h"

#include "GLDebugDrawer.h"
#include "Hexapod.h"


#ifndef M_PI
#define M_PI       3.14159265358979323846
#endif

#ifndef M_PI_2
#define M_PI_2     1.57079632679489661923
#endif

#ifndef M_PI_4
#define M_PI_4     0.785398163397448309616
#endif

#ifndef M_PI_8
#define M_PI_8     0.5 * M_PI_4
#endif
static GLDebugDrawer gDebugDrawer;


// LOCAL FUNCTIONS

void vertex(btVector3 &v)
{
    glVertex3d(v.getX(), v.getY(), v.getZ());
}

void drawFrame(btTransform &tr)
{
    const float fSize = 1.f;

    glBegin(GL_LINES);

    // x
    glColor3f(255.f,0,0);
    btVector3 vX = tr*btVector3(fSize,0,0);
    vertex(tr.getOrigin());    vertex(vX);

    // y
    glColor3f(0,255.f,0);
    btVector3 vY = tr*btVector3(0,fSize,0);
    vertex(tr.getOrigin());    vertex(vY);

    // z
    glColor3f(0,0,255.f);
    btVector3 vZ = tr*btVector3(0,0,fSize);
    vertex(tr.getOrigin());    vertex(vZ);

    glEnd();
}

// /LOCAL FUNCTIONS



#define NUM_LEGS 6
#define BODYPART_COUNT 3 * NUM_LEGS + 1
#define JOINT_COUNT BODYPART_COUNT - 1

class TestRig
{
    btDynamicsWorld*    m_ownerWorld;
    btCollisionShape*    m_shapes[BODYPART_COUNT];
    btRigidBody*        m_bodies[BODYPART_COUNT];
    btTypedConstraint*    m_joints[JOINT_COUNT];

    btRigidBody* localCreateRigidBody (btScalar mass, const btTransform& startTransform, btCollisionShape* shape)
    {
        bool isDynamic = (mass != 0.f);

        btVector3 localInertia(0,0,0);
        if (isDynamic)
            shape->calculateLocalInertia(mass,localInertia);

        btDefaultMotionState* myMotionState = new btDefaultMotionState(startTransform);
        btRigidBody::btRigidBodyConstructionInfo rbInfo(mass,myMotionState,shape,localInertia);
        btRigidBody* body = new btRigidBody(rbInfo);

        m_ownerWorld->addRigidBody(body);

        return body;
    }


public:
    TestRig (btDynamicsWorld* ownerWorld, const btVector3& positionOffset, bool bFixed)
        : m_ownerWorld (ownerWorld)
    {
        btVector3 vUp(0, 1, 0);

        //
        // Setup geometry
        //
        float fBodySize  = 0.25f;
        float fLegLength = 0.45f;
        float fForeLegLength = 0.75f;
        m_shapes[0] = new btCapsuleShape(btScalar(fBodySize), btScalar(0.10));
        int i;
        for ( i=0; i<NUM_LEGS; i++)
        {
            m_shapes[1 + 3*i] = new btCapsuleShape(btScalar(0.10), btScalar(fLegLength));
            m_shapes[2 + 3*i] = new btCapsuleShape(btScalar(0.08), btScalar(fForeLegLength));
            m_shapes[3 + 3*i] = new btCapsuleShape(btScalar(0.08), btScalar(0.04));
        }

        //
        // Setup rigid bodies
        //
        float fHeight = 1;
        btTransform offset; offset.setIdentity();
        offset.setOrigin(positionOffset);        

        // root
        btVector3 vRoot = btVector3(btScalar(0.), btScalar(fHeight), btScalar(0.));
        btTransform transform;
        transform.setIdentity();
        transform.setOrigin(vRoot);
        if (bFixed)
        {
            m_bodies[0] = localCreateRigidBody(btScalar(0.), offset*transform, m_shapes[0]);
        } else
        {
            m_bodies[0] = localCreateRigidBody(btScalar(1), offset*transform, m_shapes[0]);
        }
        // legs
        for ( i=0; i<NUM_LEGS; i++)
        {
            float fAngle = 2 * M_PI * i / NUM_LEGS;
            float fSin = sin(fAngle);
            float fCos = cos(fAngle);

            transform.setIdentity();
            btVector3 vBoneOrigin = btVector3(btScalar(fCos*(fBodySize+0.5*fLegLength)), btScalar(fHeight), btScalar(fSin*(fBodySize+0.5*fLegLength)));
            transform.setOrigin(vBoneOrigin);

            // thigh
            btVector3 vToBone = (vBoneOrigin - vRoot).normalize();
            btVector3 vAxis = vToBone.cross(vUp);            
            transform.setRotation(btQuaternion(vAxis, M_PI_2));
            m_bodies[1+3*i] = localCreateRigidBody(btScalar(0.01), offset*transform, m_shapes[1+3*i]);

            // shin
            transform.setIdentity();
            transform.setOrigin(btVector3(btScalar(fCos*(fBodySize+fLegLength)), btScalar(fHeight-0.5*fForeLegLength), btScalar(fSin*(fBodySize+fLegLength))));
            m_bodies[2+3*i] = localCreateRigidBody(btScalar(0.01), offset*transform, m_shapes[2+3*i]);

            // hip joint
            transform.setIdentity();
            transform.setOrigin(btVector3(btScalar(fCos*fBodySize), btScalar(fHeight), btScalar(fSin*fBodySize)));
            m_bodies[3+3*i] = localCreateRigidBody(btScalar(0.01), offset*transform, m_shapes[3+3*i]);
        }

        // Setup some damping on the m_bodies
        for (i = 0; i < BODYPART_COUNT; ++i)
        {
            // m_bodies[i]->setDamping(0.05, 0.05);
            // m_bodies[i]->setDeactivationTime(0.8);
            // //m_bodies[i]->setSleepingThresholds(1.6, 2.5);
            // m_bodies[i]->setSleepingThresholds(0.5f, 0.5f);
        }
        // m_bodies[0]->setDamping(0.85, 1.85);


        //
        // Setup the constraints
        //
        btHingeConstraint* hingeC;
        //btConeTwistConstraint* coneC;

        btTransform localA, localB, localC;

        for ( i=0; i<NUM_LEGS; i++)
        {
            float fAngle = 2 * M_PI * i / NUM_LEGS;
            float fSin = sin(fAngle);
            float fCos = cos(fAngle);

            // hip joints 1
            localA.setIdentity(); localB.setIdentity();
            btVector3 v1(btScalar(fCos*fBodySize), btScalar(0.), btScalar(fSin*fBodySize));
            btQuaternion q1(vUp, -fAngle);
            btQuaternion q2(btVector3(1,0,0), M_PI_2);
            btQuaternion q3 = q1 * q2;
            localA = btTransform(q3, v1);
            localB = m_bodies[3+3*i]->getWorldTransform().inverse() * m_bodies[0]->getWorldTransform() * localA;
            hingeC = new btHingeConstraint(*m_bodies[0], *m_bodies[3+3*i], localA, localB);
            hingeC->setLimit(btScalar(-M_PI_8), btScalar(M_PI_8));
            m_joints[2+3*i] = hingeC;
            hingeC->enableMotor(false);
            hingeC->setLimit(0,0);
            m_ownerWorld->addConstraint(m_joints[2+3*i], true);
            // hip joints 2
            localA.setIdentity(); localB.setIdentity();
            localA = btTransform(q1, v1);
            localB = m_bodies[1+3*i]->getWorldTransform().inverse() * m_bodies[0]->getWorldTransform() * localA;
            localC = m_bodies[3+3*i]->getWorldTransform().inverse() * m_bodies[0]->getWorldTransform() * localA;
            hingeC = new btHingeConstraint(*m_bodies[3+3*i], *m_bodies[1+3*i], localC, localB);
            hingeC->setLimit(btScalar(-M_PI_8), btScalar(M_PI_8));
            //hingeC->setLimit(btScalar(-0.1), btScalar(0.1));
            m_joints[3*i] = hingeC;
            hingeC->enableMotor(false);
            hingeC->setLimit(0,0);
            m_ownerWorld->addConstraint(m_joints[3*i], true);

            // knee joints
            localA.setIdentity(); localB.setIdentity(); localC.setIdentity();
            localA.getBasis().setEulerZYX(0,-fAngle,0);    localA.setOrigin(btVector3(btScalar(fCos*(fBodySize+fLegLength)), btScalar(0.), btScalar(fSin*(fBodySize+fLegLength))));
            localB = m_bodies[1+3*i]->getWorldTransform().inverse() * m_bodies[0]->getWorldTransform() * localA;
            localC = m_bodies[2+3*i]->getWorldTransform().inverse() * m_bodies[0]->getWorldTransform() * localA;
            hingeC = new btHingeConstraint(*m_bodies[1+3*i], *m_bodies[2+3*i], localB, localC);
            //hingeC->setLimit(btScalar(-0.01), btScalar(0.01));
            hingeC->setLimit(btScalar(-M_PI_8), btScalar(M_PI_8));
            m_joints[1+3*i] = hingeC;
            hingeC->enableMotor(false);
            hingeC->setLimit(0,0);
            m_ownerWorld->addConstraint(m_joints[1+3*i], true);
        }
    }

    virtual    ~TestRig ()
    {
        int i;

        // Remove all constraints
        for ( i = 0; i < JOINT_COUNT; ++i)
        {
            m_ownerWorld->removeConstraint(m_joints[i]);
            delete m_joints[i]; m_joints[i] = 0;
        }

        // Remove all bodies and shapes
        for ( i = 0; i < BODYPART_COUNT; ++i)
        {
            m_ownerWorld->removeRigidBody(m_bodies[i]);
            
            delete m_bodies[i]->getMotionState();

            delete m_bodies[i]; m_bodies[i] = 0;
            delete m_shapes[i]; m_shapes[i] = 0;
        }
    }

    btTypedConstraint** GetJoints() {return &m_joints[0];}

};



void motorPreTickCallback (btDynamicsWorld *world, btScalar timeStep)
{
    Hexapod* hexapod = (Hexapod*)world->getWorldUserInfo();

    hexapod->setMotorTargets(timeStep);
    
}



void Hexapod::initPhysics()
{
    setTexturing(true);
    setShadows(true);

    // Setup the basic world

    m_Time = 0;
    m_fCyclePeriod = 2000.f; // in milliseconds

//    m_fMuscleStrength = 0.05f;
    // new SIMD solver for joints clips accumulated impulse, so the new limits for the motor
    // should be (numberOfsolverIterations * oldLimits)
    // currently solver uses 10 iterations, so:
    m_fMuscleStrength = 0.5f;

    setCameraDistance(btScalar(5.));

    m_collisionConfiguration = new btDefaultCollisionConfiguration();

    m_dispatcher = new btCollisionDispatcher(m_collisionConfiguration);

    btVector3 worldAabbMin(-10000,-10000,-10000);
    btVector3 worldAabbMax(10000,10000,10000);
    m_broadphase = new btAxisSweep3 (worldAabbMin, worldAabbMax);

    m_solver = new btSequentialImpulseConstraintSolver;

    m_dynamicsWorld = new btDiscreteDynamicsWorld(m_dispatcher,m_broadphase,m_solver,m_collisionConfiguration);

    m_dynamicsWorld->setInternalTickCallback(motorPreTickCallback,this,true);
    m_dynamicsWorld->setDebugDrawer(&gDebugDrawer);

    // Setup a big ground box
    {
        btCollisionShape* groundShape = new btBoxShape(btVector3(btScalar(200.),btScalar(10.),btScalar(200.)));
        m_collisionShapes.push_back(groundShape);
        btTransform groundTransform;
        groundTransform.setIdentity();
        groundTransform.setOrigin(btVector3(0,-10,0));
        localCreateRigidBody(btScalar(0.),groundTransform,groundShape);
    }

    // Spawn one ragdoll
    btVector3 startOffset(0,0,0);
    spawnTestRig(startOffset, false);
    startOffset = btVector3(-3,0,0);
    spawnTestRig(startOffset, true);
    clientResetScene();        
}


void Hexapod::spawnTestRig(const btVector3& startOffset, bool bFixed)
{
    TestRig* rig = new TestRig(m_dynamicsWorld, startOffset, bFixed);
    m_rigs.push_back(rig);
}

void    PreStep()
{

}




void Hexapod::setMotorTargets(btScalar deltaTime)
{

    float ms = deltaTime*1000000.;
    float minFPS = 1000000.f/60.f;
    if (ms > minFPS)
        ms = minFPS;

    m_Time += ms;

    //
    // set per-frame sinusoidal position targets using angular motor (hacky?)
    //    
    for (int r=0; r<m_rigs.size(); r++)
    {
        for (int i=0; i<3*NUM_LEGS; i++)
        {
            btHingeConstraint* hingeC = static_cast<btHingeConstraint*>(m_rigs[r]->GetJoints()[i]);
            btScalar fCurAngle      = hingeC->getHingeAngle();
            
            btScalar fTargetPercent = (int(m_Time / 1000) % int(m_fCyclePeriod)) / m_fCyclePeriod;
            btScalar fTargetAngle   = 0.5 * (1 + sin(2 * M_PI * fTargetPercent));
            btScalar fTargetLimitAngle = hingeC->getLowerLimit() + fTargetAngle * (hingeC->getUpperLimit() - hingeC->getLowerLimit());
            btScalar fAngleError  = fTargetLimitAngle - fCurAngle;
            btScalar fDesiredAngularVel = 1000000.f * fAngleError/ms;
            // hingeC->enableAngularMotor(true, fDesiredAngularVel, m_fMuscleStrength);
            hingeC->setLimit(0,0);
            hingeC->enableMotor(false);
        }
    }

    
}

void Hexapod::clientMoveAndDisplay()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); 

    //simple dynamics world doesn't handle fixed-time-stepping
    float deltaTime = 1.0f/60.0f;
    

    if (m_dynamicsWorld)
    {
        m_dynamicsWorld->stepSimulation(deltaTime,1000, 0.002);
        m_dynamicsWorld->debugDrawWorld();
    }

    renderme(); 

    for (int i=2; i>=0 ;i--)
    {
        btCollisionObject* obj = m_dynamicsWorld->getCollisionObjectArray()[i];
        btRigidBody* body = btRigidBody::upcast(obj);
        drawFrame(body->getWorldTransform());
    }

    glFlush();

    glutSwapBuffers();
}

void Hexapod::displayCallback()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); 

    if (m_dynamicsWorld)
        m_dynamicsWorld->debugDrawWorld();

    renderme();

    glFlush();
    glutSwapBuffers();
}

void Hexapod::keyboardCallback(unsigned char key, int x, int y)
{
    switch (key)
    {
    case '+': case '=':
        m_fCyclePeriod /= 1.1f;
        if (m_fCyclePeriod < 1.f)
            m_fCyclePeriod = 1.f;
        break;
    case '-': case '_':
        m_fCyclePeriod *= 1.1f;
        break;
    case '[':
        m_fMuscleStrength /= 1.1f;
        break;
    case ']':
        m_fMuscleStrength *= 1.1f;
        break;
    case 27:
        exitPhysics();
        exit(0);
        break;
    default:
        DemoApplication::keyboardCallback(key, x, y);
    }    
}



void Hexapod::exitPhysics()
{

    int i;

    for (i=0;i<m_rigs.size();i++)
    {
        TestRig* rig = m_rigs[i];
        delete rig;
    }

    //cleanup in the reverse order of creation/initialization

    //remove the rigidbodies from the dynamics world and delete them
    
    for (i=m_dynamicsWorld->getNumCollisionObjects()-1; i>=0 ;i--)
    {
        btCollisionObject* obj = m_dynamicsWorld->getCollisionObjectArray()[i];
        btRigidBody* body = btRigidBody::upcast(obj);
        if (body && body->getMotionState())
        {
            delete body->getMotionState();
        }
        m_dynamicsWorld->removeCollisionObject( obj );
        delete obj;
    }

    //delete collision shapes
    for (int j=0;j<m_collisionShapes.size();j++)
    {
        btCollisionShape* shape = m_collisionShapes[j];
        delete shape;
    }

    //delete dynamics world
    delete m_dynamicsWorld;

    //delete solver
    delete m_solver;

    //delete broadphase
    delete m_broadphase;

    //delete dispatcher
    delete m_dispatcher;

    delete m_collisionConfiguration;    
}