#ifndef HEXAPOD_H
#define HEXAPOD_H

#include "GlutDemoApplication.h"
#include "LinearMath/btAlignedObjectArray.h"
#include "zmq.hpp"

class btBroadphaseInterface;
class btCollisionShape;
class btOverlappingPairCache;
class btCollisionDispatcher;
class btConstraintSolver;
struct btCollisionAlgorithmCreateFunc;
class btDefaultCollisionConfiguration;

class Hexapod : public GlutDemoApplication
{
    float m_Time;
    float m_fCyclePeriod; // in milliseconds
    float m_fMuscleStrength;

    btAlignedObjectArray<class HexapodRig*> m_rigs;

    //keep the collision shapes, for deletion/cleanup
    btAlignedObjectArray<btCollisionShape*>    m_collisionShapes;

    btBroadphaseInterface*    m_broadphase;

    btCollisionDispatcher*    m_dispatcher;

    btConstraintSolver*    m_solver;

    btDefaultCollisionConfiguration* m_collisionConfiguration;

public:
    void initPhysics();

    void exitPhysics();

    virtual ~Hexapod()
    {
        exitPhysics();
    }

    void spawnHexapodRig(const btVector3& startOffset, bool bFixed);

    virtual void clientMoveAndDisplay();

    virtual void displayCallback();

    virtual void keyboardCallback(unsigned char key, int x, int y);

    static DemoApplication* Create()
    {
        Hexapod* demo = new Hexapod();
        demo->myinit();
        demo->initPhysics();
        zmq::context_t context (1);
        zmq::socket_t publisher (context, ZMQ_PUB);
        publisher.bind("tcp://*:5556");
        return demo;
    }
    void setServoPercent(int rigId, int jointId, btScalar targetPercent, float deltaMs);
    void setMotorTargets(btScalar deltaTime);

};


#endif
