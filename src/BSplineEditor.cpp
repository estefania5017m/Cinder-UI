#include "BSplineEditor.h"

using namespace reza::ui;
using namespace cinder;
using namespace std;

BSplineEditor::BSplineEditor( string name, BSpline2f spline, const Format &format ) : ControlWithLabel(), mUseRef( false ), mSplineRef( new BSpline2f( spline ) ), mCallbackFn( nullptr ), mFormat( format )
{
    setName( name );
    setSpline( spline );
}

BSplineEditor::~BSplineEditor()
{
    if( !mUseRef )
    {
        delete mSplineRef;
    }
}

void BSplineEditor::setup()
{
    if( !mLabelRef && mFormat.mLabel )
    {
        mLabelRef = Label::create( mName + "_LABEL", mName, mFormat.mFontSize );
        mLabelRef->setOrigin( vec2( 0.0f, getHeight() ) );
        addSubView( mLabelRef );
    }
    View::setup();
}

void BSplineEditor::trigger( bool recursive )
{
    if( mCallbackFn )
    {
        mCallbackFn( getSpline() );
    }
    Control::trigger( recursive );
}

JsonTree BSplineEditor::save()
{
    JsonTree tree = View::save();
//    tree.addChild( JsonTree( "DEGREE", getDegree() ) );
//    tree.addChild( JsonTree( "LOOP",  isLoop() ) );
//    tree.addChild( JsonTree( "OPEN",  isOpen() ) );
    JsonTree subtree = JsonTree::makeArray( "POINTS" );
    for( auto& it : mControlPoints )
    {
        vec2 mapped = norm( it );
        JsonTree subsubtree;
        subsubtree.addChild( JsonTree( "X", mapped.x ) );
        subsubtree.addChild( JsonTree( "Y", mapped.y ) );
        subtree.addChild( subsubtree );
    }
    if( subtree.getNumChildren() )
    {
        tree.addChild( subtree );
    }
    return tree;
}

void BSplineEditor::load( const ci::JsonTree &data )
{
//    if( data.hasChild( "OPEN" ) && data.hasChild( "LOOP" ) && data.hasChild( "DEGREE" ) )
//    {
//        setProperties( data.getValueForKey<int>( "DEGREE" ),
//                       data.getValueForKey<bool>( "LOOP" ),
//                       data.getValueForKey<bool>( "OPEN" ) );
//    }
    if( data.hasChild( "POINTS" ) )
    {
        auto pts = data.getChild( "POINTS" );
        int total = pts.getNumChildren();
        if( mControlPoints.size() < total )
        {
            mControlPoints.resize( total );
        }
        for( int i = 0; i < total; i++ )
        {
            auto child = pts.getChild( i );
            mControlPoints[i] = expand( vec2( child.getValueForKey<float>( "X" ), child.getValueForKey<float>( "Y" ) ) ); 
        }
        updateSplineRef();
        trigger();
    }
    View::load( data );
}

void BSplineEditor::setSpline( BSpline2f spline )
{
    mOpen = spline.isOpen();
    mLoop = spline.isLoop();
    mDegree = spline.getDegree();
    mControlPoints.clear();
    int total = spline.getNumControlPoints();
    for( int i = 0; i < total; i++ )
    {
        mControlPoints.emplace_back( spline.getControlPoint( i ) );
    }
    updateSplineRef();
}

void BSplineEditor::setSplineRef( BSpline2f *spline )
{
    if( !mUseRef )
    {
        mUseRef = false;
        delete mSplineRef;
    }
    mSplineRef = spline;
    mDegree = mSplineRef->getDegree();
    mLoop = mSplineRef->isLoop();
    mOpen = mSplineRef->isOpen();
}

BSpline2f BSplineEditor::getSpline()
{
    return *mSplineRef;
}

int BSplineEditor::getDegree()
{
    return mDegree;
}

bool BSplineEditor::isLoop()
{
    return mLoop;
}

bool BSplineEditor::isOpen()
{
    return mOpen;
}

void BSplineEditor::setDegree( int degree )
{
    if( mDegree != degree && ( degree > 0 ) && ( degree <= ( mControlPoints.size() - 1 ) ) )
    {
        mDegree = degree;
        updateSplineRef( true );
        trigger();
    }
}

void BSplineEditor::setLoop( bool loop )
{
    if( mLoop != loop )
    {
        mLoop = loop;
        updateSplineRef( true );
        trigger();
    }
}

void BSplineEditor::setOpen( bool open )
{
    if( mOpen != open )
    {
        mOpen = open;
        updateSplineRef( true );
        trigger();
    }
}

void BSplineEditor::setProperties( int degree, bool loop, bool open )
{
    mDegree = degree;
    mLoop = loop;
    mOpen = open;
    updateSplineRef( true );
    trigger();
}

void BSplineEditor::updateSplineRef( bool force )
{
    if( ( mControlPoints.size() != mSplineRef->getNumControlPoints() ) || force )
    {
        delete mSplineRef;
        mSplineRef = new BSpline2f( mControlPoints, mDegree, mLoop, mOpen );
    }
    else
    {
        int total = mControlPoints.size();
        for( int i = 0; i < total; i++ )
        {
            mSplineRef->setControlPoint( i, mControlPoints[i] );
        }
    }
    setNeedsDisplay();
}

void BSplineEditor::setCallback( const std::function<void(BSpline2f)> &callback )
{
    mCallbackFn = callback;
}

void BSplineEditor::setMax( vec2 max, bool keepValueTheSame )
{
    setMinAndMax( max, mFormat.mMin, keepValueTheSame );
}

vec2 BSplineEditor::getMax()
{
    return mFormat.mMax;
}

void BSplineEditor::setMin( vec2 min, bool keepValueTheSame )
{
    setMinAndMax( mFormat.mMax, min, keepValueTheSame );
}

vec2 BSplineEditor::getMin()
{
    return mFormat.mMin;
}

void BSplineEditor::setMinAndMax( vec2 min, vec2 max, bool keepValueTheSame )
{
    if( !keepValueTheSame )
    {
        for( auto &it : mControlPoints )
        {
            it.x = lmap<float>( it.x, mFormat.mMin.x, mFormat.mMax.x, min.x, max.x );
            it.y = lmap<float>( it.y, mFormat.mMin.y, mFormat.mMax.y, min.y, max.y );
        }
        updateSplineRef();
        trigger();
    }
    
    mFormat.mMax = max;
    mFormat.mMin = min;
}

std::vector<RenderData> BSplineEditor::render()
{
    vector<RenderData> data;
    drawBounds( data, ( mDrawBounds && mVisible ) ? mColorBounds : mColorClear );
    drawBoundsOutline( data, ( mDrawBoundsOutline && mVisible ) ? mColorBoundsOutline : mColorClear );
    drawBack( data, ( mDrawBack && mVisible ) ? mColorBack : mColorClear );
    drawFill( data, ( mDrawFill && mVisible ) ? mColorFill : mColorClear );
    drawFillHighlight( data, ( mDrawFillHighlight && mVisible ) ? mColorFillHighlight : mColorClear );
    drawOutline( data, ( mDrawOutline && mVisible ) ? mColorOutline : mColorClear );
    drawOutlineHighlight( data, ( mDrawOutlineHighlight && mVisible ) ? mColorOutlineHighlight : mColorClear );
    for( int i = data.size(); i < 17982; i++ )
    {
        data.emplace_back( RenderData() );
    }
    return data;
}

void BSplineEditor::drawOutline( std::vector<RenderData> &data, const ci::ColorA &color )
{
    Control::drawOutline( data, color );
}

void BSplineEditor::drawOutlineHighlight( std::vector<RenderData> &data, const ci::ColorA &color )
{
    Control::drawOutline( data, color );
}

void BSplineEditor::drawFill( std::vector<RenderData> &data, const ci::ColorA &color )
{
    if( mSplineRef != nullptr && mSplineRef->getNumControlPoints() > 0 )
    {
        vec2 last = vec2( 0.0 );
        vec2 curr = vec2( 0.0 );
        for( int i = 0; i <= mFormat.mResolution; ++i )
        {
            float t = i / (float) mFormat.mResolution;
            curr = map( mSplineRef->getPosition( t ) );
            if( i != 0 )
            {
                addLine( data, color, last, curr );
            }
            last = curr;
        }
        
        for( auto it : mControlPoints )
        {
            vec2 curr = map( it );
            addPoint( data, color, curr, 3.0 );
        }
    }
    
    if( mHitIndex != -1 )
    {
        addPoint( data, ColorA(1.0, 0.0, 0.0, 1.0), map( mControlPoints[mHitIndex] ), 4.0 );
    }
    else
    {
        addPoint( data, mColorClear, vec2( 0.0 ), 4.0 );
    }
}

void BSplineEditor::drawFillHighlight( std::vector<RenderData> &data, const ci::ColorA &color )
{
    drawFill( data, color );
}

void BSplineEditor::input( const ci::app::MouseEvent& event )
{
    vec2 hp = getHitPercent( event.getPos() );
    hp.x = min( max( hp.x, 0.0f ), 1.0f );
    hp.y = min( max( hp.y, 0.0f ), 1.0f );
    hp = vec2( lmap<float>( hp.x, 0.0, 1.0, mFormat.mMin.x, mFormat.mMax.x ),
              lmap<float>( hp.y, 0.0, 1.0, mFormat.mMin.y, mFormat.mMax.y ) );
    
    if( event.isShiftDown() || mFormat.mSticky )
    {
        hp.x = ceil( hp.x / mFormat.mStickyValue ) * mFormat.mStickyValue;
        hp.y = ceil( hp.y / mFormat.mStickyValue ) * mFormat.mStickyValue;
    }
    
    if( getState() == State::NORMAL || getState() == State::OVER ) {
        mHitIndex = -1;
    } else if( mHitIndex == -1 ) {
        float distance = 100000.0;
        int index = -1;
        for( int i = 0; i < mControlPoints.size(); i++ )
        {
            float len = length( hp - mControlPoints[i] );
            if( len < distance ) {
                distance = len; index = i;
            }
        }
        if( distance < ( length( mFormat.mMax - mFormat.mMin ) * mFormat.mThreshold ) ) {
            mHitIndex = index;
            if( ( event.isRight() || event.isMetaDown() ) && ( mControlPoints.size() - 1 ) > mSplineRef->getDegree() )
            {
                mControlPoints.erase( mControlPoints.begin() + mHitIndex );
                updateSplineRef();
                mHitIndex = -1;
            }
        } else {
            mHitIndex = mControlPoints.size();
            mControlPoints.push_back( hp );
            updateSplineRef();
        }
    }
    
    if( mHitIndex != -1 ) {
        mControlPoints[mHitIndex] = hp;
        updateSplineRef();
    }
}

void BSplineEditor::mouseDown( ci::app::MouseEvent &event )
{
    View::mouseDown( event );
    if( isHit( event.getPos() ) )
    {
        mHit = true;
        setState( State::DOWN );
        input( event );
        if( (int)mTrigger & (int)Trigger::BEGIN )
        {
            trigger();
        }
    }
    else
    {
        setState( State::NORMAL );
    }
}


void BSplineEditor::mouseUp( ci::app::MouseEvent &event )
{
    View::mouseUp( event );
    if( mHit )
    {
#ifdef CINDER_COCOA_TOUCH
        setState( State::NORMAL );
#else
        if( isHit( event.getPos() ) )
        {
            setState( State::OVER );
        }
        else
        {
            setState( State::NORMAL );
        }
#endif
        input( event );
        if( (int)mTrigger & (int)Trigger::END )
        {
            trigger();
        }
    }
    mHit = false;
}


void BSplineEditor::mouseWheel( ci::app::MouseEvent &event )
{
    View::mouseWheel( event );
    if( mHit && mHitIndex != -1 )
    {
        float value = mSplineRef->getKnot( mHitIndex );
        value += event.getWheelIncrement()*0.1;
        cout << event.getWheelIncrement() << endl;
        value = min( max( value, 0.0f ), 1.0f );
        mSplineRef->setKnot( mHitIndex, value );
        cout << "HIT: " << mHitIndex << " VALUE: " << value << endl;
    }
}

void BSplineEditor::mouseMove( ci::app::MouseEvent &event )
{
    View::mouseMove( event );
    if( isHit( event.getPos() ) )
    {
        setState( State::OVER );
    }
    else
    {
        setState( State::NORMAL );
    }
}


void BSplineEditor::mouseDrag( ci::app::MouseEvent &event )
{
    View::mouseDrag( event );
    if( mHit )
    {
        setState( State::DOWN );
        input( event );
        if( (int)mTrigger & (int)Trigger::CHANGE )
        {
            trigger();
        }
    }
    else
    {
        setState( State::NORMAL );
    }
}

vec2 BSplineEditor::map( const vec2& pt )
{
    vec2 result;
    result.x = lmap<float>( pt.x, mFormat.mMin.x, mFormat.mMax.x, mHitRect.getUpperLeft().x, mHitRect.getLowerRight().x );
    result.y = lmap<float>( pt.y, mFormat.mMin.y, mFormat.mMax.y, mHitRect.getUpperLeft().y, mHitRect.getLowerRight().y );
    return result;
}

vec2 BSplineEditor::norm( const vec2& pt )
{
    vec2 result;
    result.x = lmap<float>( pt.x, mFormat.mMin.x, mFormat.mMax.x, 0.0f, 1.0f );
    result.y = lmap<float>( pt.y, mFormat.mMin.y, mFormat.mMax.y, 0.0f, 1.0f );
    return result;
}

vec2 BSplineEditor::expand( const vec2& pt )
{
    vec2 result;
    result.x = lmap<float>( pt.x, 0.0f, 1.0f, mFormat.mMin.x, mFormat.mMax.x );
    result.y = lmap<float>( pt.y, 0.0f, 1.0f, mFormat.mMin.y, mFormat.mMax.y );
    return result;
}