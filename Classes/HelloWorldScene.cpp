#include "HelloWorldScene.h"

#include <stdio.h>
#define PTM_RATIO 32.0

using namespace cocos2d;



CCScene* HelloWorld::scene()
{
    CCScene * scene = NULL;
    do 
    {
        // 'scene' is an autorelease object
        scene = CCScene::create();
        CC_BREAK_IF(! scene);

        // 'layer' is an autorelease object
        HelloWorld *layer = HelloWorld::create();
        CC_BREAK_IF(! layer);

        // add layer as a child to scene
        scene->addChild(layer);
    } while (0);

    // return the scene
    return scene;
}

void HelloWorld::ccTouchesEnded(cocos2d::CCSet* touches, cocos2d::CCEvent* event)
{
	kick();
}

// on "init" you need to initialize your instance
bool HelloWorld::init()
{
    bool bRet = false;
    do 
	{
        CC_BREAK_IF(! CCLayer::init());

		 screenSize = CCDirector::sharedDirector()->getWinSize();

		 b2Vec2 gravity( 0, -10 );
		 m_world = new b2World(gravity);
		m_world->SetContinuousPhysics(true);
		m_world->SetContactListener(&myListener);
		 this->setTouchEnabled(true);

		
		initPhysics();
		//scheduleUpdate();

		this->schedule(schedule_selector(HelloWorld::update));
		
		
		/*box = CCSprite::create("wooden_box.jpg");
		 CC_BREAK_IF(! box);

		 ball = CCSprite::create("Rock-01.png");
		 CC_BREAK_IF(! ball);
	    box->setScale(0.09);

		ball->setScale(1.0);

		addChild(box);
		addChild(ball);*/
		
		///////////////////////////////////////////////////////////////////////////////////
		//For Box2d Debug Drawing//
		m_DebugDraw = new b2DebugDraw(PTM_RATIO);
		
		m_world->SetDebugDraw(m_DebugDraw);
		uint32 flags = 0;
		flags += b2Draw::e_shapeBit;
		
		m_DebugDraw->SetFlags(flags);
		
		
		//////////////////////////////////////////////////////////////////////////////////
		//myListener = new BouyancyContactListener();
		
        bRet = true;
    } while (0);

    return bRet;
}

void HelloWorld::kick()
{
	b2Vec2 force = b2Vec2(30,30);
	_boat->ApplyLinearImpulse(force,_boat->GetPosition());

}
void HelloWorld::update(float dt)
{
	m_world->Step(dt,10,10);
	for(b2Body *b = m_world->GetBodyList(); b; b = b->GetNext())
	{
		if(b->GetUserData() != NULL)
		{
			CCSprite *objectData = (CCSprite *)b->GetUserData();
			objectData->setPosition(ccp(b->GetPosition().x * PTM_RATIO,b->GetPosition().y * PTM_RATIO));
		    objectData->setRotation( -1 * CC_RADIANS_TO_DEGREES(b->GetAngle()));

			
	  }
	
	}


	    std::set<fixturePair>::iterator it = myListener.m_fixturePairs.begin();
        std::set<fixturePair>::iterator end = myListener.m_fixturePairs.end();
        while (it != end) {

            //fixtureA is the fluid
            b2Fixture* fixtureA = it->first;
            b2Fixture* fixtureB = it->second;

            float density = fixtureA->GetDensity();

            std::vector<b2Vec2> intersectionPoints;
            if (myListener.findIntersectionOfFixtures(fixtureA, fixtureB, intersectionPoints) ) {

                //find centroid
                float area = 0;
                b2Vec2 centroid = myListener.ComputeCentroid( intersectionPoints, area);

                //apply buoyancy force
                float displacedMass = fixtureA->GetDensity() * area;
                b2Vec2 gravity( 0, -10 );
                b2Vec2 buoyancyForce = displacedMass * -gravity;
                fixtureB->GetBody()->ApplyForce( buoyancyForce, centroid);

                /*
                //simple drag
                //find relative velocity between object and fluid
                b2Vec2 velDir = fixtureB->GetBody()->GetLinearVelocityFromWorldPoint( centroid ) -
                        fixtureA->GetBody()->GetLinearVelocityFromWorldPoint( centroid );
                float vel = velDir.Normalize();

                float dragMod = 1;//adjust as desired
                float dragMag = fixtureA->GetDensity() * vel * vel;
                b2Vec2 dragForce = dragMod * dragMag * -velDir;
                fixtureB->GetBody()->ApplyForce( dragForce, centroid,true );
                float angularDrag = area * -fixtureB->GetBody()->GetAngularVelocity();
                fixtureB->GetBody()->ApplyTorque( angularDrag );
                */

                //apply complex drag
                float dragMod = 0.25f;//adjust as desired
                float liftMod = 0.25f;//adjust as desired
                float maxDrag = 2000;//adjust as desired
                float maxLift = 500;//adjust as desired
                for (int i = 0; i < intersectionPoints.size(); i++) {
                    b2Vec2 v0 = intersectionPoints[i];
                    b2Vec2 v1 = intersectionPoints[(i+1)%intersectionPoints.size()];
                    b2Vec2 midPoint = 0.5f * (v0+v1);

                    //find relative velocity between object and fluid at edge midpoint
                    b2Vec2 velDir = fixtureB->GetBody()->GetLinearVelocityFromWorldPoint( midPoint ) -
                            fixtureA->GetBody()->GetLinearVelocityFromWorldPoint( midPoint );
                    float vel = velDir.Normalize();

                    b2Vec2 edge = v1 - v0;
                    float edgeLength = edge.Normalize();
                    b2Vec2 normal = b2Cross(-1,edge);
                    float dragDot = b2Dot(normal, velDir);
                    if ( dragDot < 0 )
                        continue;//normal points backwards - this is not a leading edge

                    //apply drag
                    float dragMag = dragDot * dragMod * edgeLength * density * vel * vel;
                    dragMag = b2Min( dragMag, maxDrag );
                    b2Vec2 dragForce = dragMag * -velDir;
                    fixtureB->GetBody()->ApplyForce( dragForce, midPoint);

                    //apply lift
                    float liftDot = b2Dot(edge, velDir);
                    float liftMag =  dragDot * liftDot * liftMod * edgeLength * density * vel * vel;
                    liftMag = b2Min( liftMag, maxLift );
                    b2Vec2 liftDir = b2Cross(1,velDir);
                    b2Vec2 liftForce = liftMag * liftDir;
                    fixtureB->GetBody()->ApplyForce( liftForce, midPoint );
                }

             //draw debug info
                glColor3f(0,1,1);
                glLineWidth(2);
                glBegin(GL_LINE_LOOP);
                for (int i = 0; i < intersectionPoints.size(); i++)
                    glVertex2f( intersectionPoints[i].x, intersectionPoints[i].y );
                glEnd();
                glLineWidth(1);
                
                //line showing buoyancy force
                /*if ( area > 0 ) {
                    glBegin(GL_LINES);
                    glVertex2f( centroid.x, centroid.y );
                    glVertex2f( centroid.x, centroid.y + area );
                    glEnd();
                }*/
            }

            ++it;
        }	


	
}
void HelloWorld::initPhysics()
{

	

	//sphereDef.type = b2_dynamicBody;
	//sphereDef.userData = ball;
	//sphereDef.position.Set(90.0f/PTM_RATIO,200.0f/PTM_RATIO);
	//_sphere    = m_world->CreateBody(&sphereDef);
	//b2CircleShape sphereShape;

	//sphereShape.m_radius = 10.0f /PTM_RATIO;
	//b2FixtureDef sphereFix;
	//sphereFix.shape = &sphereShape;
	//sphereFix.restitution = 1.0f;
	//sphereFix.density = 20.0f;
	//sphereFix.isSensor = false;

	//_sphere->CreateFixture(&sphereFix);
	// 

	//boxBB = box->boundingBox();
	//	 boxDef.userData = box;
	//	 boxDef.type = b2_dynamicBody;
	//	 boxDef.linearVelocity.Set(0.0f,0.0f);
	//	 boxDef.angularVelocity = 0.0f;
	//	 boxDef.linearDamping = 0.0f;
	//	 boxDef.angularDamping = 0.0f;
	//	 boxDef.allowSleep = bool(4);
	//	 boxDef.awake = bool(2);
	//	 boxDef.fixedRotation = bool(0);
	//	 boxDef.bullet = bool(0);
	//	 boxDef.active = bool(32);
	//	 boxDef.gravityScale = 1.0f;
	//	 boxDef.position.Set(350.0f / PTM_RATIO,300.0f / PTM_RATIO);
	//	 _box       = m_world->CreateBody(&boxDef);
	//	 b2PolygonShape boxShape;

	//	 boxShape.SetAsBox(30  / PTM_RATIO, 30 / PTM_RATIO);
	//	// boxShape.SetAsBox(boxBB.size.width /PTM_RATIO,boxBB.size.height /PTM_RATIO,b2Vec2(0.0f,0.0f),0.0f);
	//	 b2FixtureDef boxFix;
	//	 boxFix.shape = &boxShape;
	//	 boxFix.density = 1.0f;
	//	 boxFix.friction = 2.0f;
	//	 boxFix.isSensor = false;
	//	 boxFix.filter.categoryBits = uint16(1);
	//	 boxFix.filter.maskBits = uint16(65535);
	//	 boxFix.filter.groupIndex = int16(0);

	//	 boxFix.restitution = 0.0f;
	//	 _box->CreateFixture(&boxFix);

		 

		// Define the ground body.
	b2BodyDef groundBodyDef;
	groundBodyDef.position.Set(screenSize.width/2/PTM_RATIO, screenSize.height/2/PTM_RATIO); // bottom-left corner
	
	// Call the body factory which allocates memory for the ground body
	// from a pool and creates the ground box shape (also from a pool).
	// The body is also added to the world.
	b2Body* groundBody = m_world->CreateBody(&groundBodyDef);

	// Define the ground box shape.
	b2PolygonShape groundBox;
    // bottom
    groundBox.SetAsBox(screenSize.width/2/PTM_RATIO, 0, b2Vec2(0, -screenSize.height/2/PTM_RATIO), 0);
 	groundBody->CreateFixture(&groundBox, 0);
	
    // top
    groundBox.SetAsBox(screenSize.width/2/PTM_RATIO, 0, b2Vec2(0, screenSize.height/2/PTM_RATIO), 0);
    groundBody->CreateFixture(&groundBox, 0);

    // left
    groundBox.SetAsBox(0, screenSize.height/2/PTM_RATIO, b2Vec2(-screenSize.width/2/PTM_RATIO, 0), 0);
    groundBody->CreateFixture(&groundBox, 0);

    // right
    groundBox.SetAsBox(0, screenSize.height/2/PTM_RATIO, b2Vec2(screenSize.width/2/PTM_RATIO, 0), 0);
    groundBody->CreateFixture(&groundBox, 0);








 
  waterDef.type = b2_staticBody;
  waterDef.position.Set(200.0f / PTM_RATIO,3.0f / PTM_RATIO);
  waterDef.angle = 0.0f;
  waterDef.linearVelocity.Set(0.0f, 0.0f);
  waterDef.angularVelocity = 0.0;
  waterDef.linearDamping = 0.0f;
  waterDef.angularDamping = 0.0f;
  waterDef.allowSleep = true;
  waterDef.awake = false;
  waterDef.fixedRotation = false;
  waterDef.bullet = false;
  waterDef.active =true ;
  waterDef.gravityScale = 1.0f;
  _water = m_world->CreateBody(&waterDef);

  
    b2FixtureDef waterFix;
  
    waterFix.restitution = 0.0f;
    
    waterFix.isSensor = true;
    waterFix.filter.categoryBits = uint16(1);
    waterFix.filter.maskBits = uint16(65535);
    waterFix.filter.groupIndex = int16(0);
    b2PolygonShape waterShape;
    

	waterShape.SetAsBox(screenSize.width / PTM_RATIO,400.0f / PTM_RATIO,b2Vec2(0.0f,0.0f),0.0f);

    waterFix.shape = &waterShape;

    _water->CreateFixture(&waterFix);
  
 


//boat?

 // b2BodyDef boatDef;
  boatDef.type = b2_dynamicBody;
  boatDef.position.Set(420.0f /  PTM_RATIO, 410.0f  /  PTM_RATIO);
  boatDef.angle = 3.824857473373413e-01f;
  boatDef.linearVelocity.Set(0.0f, 0.0f);
  boatDef.angularVelocity = 0.0f;
  boatDef.linearDamping = 0.0f;
  boatDef.angularDamping = 0.0f;
  boatDef.allowSleep = false;
  boatDef.awake = true;
  boatDef.fixedRotation = false;
  boatDef.bullet = false;
  boatDef.active = true;
  boatDef.gravityScale = 1.0f;
   _boat = m_world->CreateBody(&boatDef);

  
    b2FixtureDef boatFix;
    boatFix.friction = 2.0f;
    boatFix.restitution = 0.0f;
    boatFix.density = 1.0f;
    boatFix.isSensor = false;
    boatFix.filter.categoryBits = uint16(1);
    boatFix.filter.maskBits = uint16(65535);
    boatFix.filter.groupIndex = int16(0);
    b2PolygonShape boatShape;
    b2Vec2 vs[8];
    vs[0].Set(2.595919609069824e+00f, -1.319291234016418e+00f);
    vs[1].Set(-2.777174711227417e+00f, 9.063234329223633e-01f);
    vs[2].Set(-1.861940383911133e+00f, -4.768543243408203e-01f);
    vs[3].Set(-1.460509777069092e+00f, -1.000003814697266e+00f);
    vs[4].Set(-9.373636245727539e-01f, -1.401435852050781e+00f);
    vs[5].Set(-3.281402587890625e-01f, -1.653779983520508e+00f);
    vs[6].Set(3.256297111511230e-01f, -1.739847183227539e+00f);
    vs[7].Set(9.794044494628906e-01f, -1.653779983520508e+00f);
    boatShape.Set(vs, 8);

    boatFix.shape = &boatShape;

    _boat->CreateFixture(&boatFix);










}



void HelloWorld::draw(void)
{
	 

    ccGLEnableVertexAttribs( kCCVertexAttribFlag_Position );

    kmGLPushMatrix();

    m_world->DrawDebugData();

    kmGLPopMatrix();

    CHECK_GL_ERROR_DEBUG();
}

    

void HelloWorld::menuCloseCallback(CCObject* pSender)
{
    // "close" menu item clicked
    CCDirector::sharedDirector()->end();
}

