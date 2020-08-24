package com.example.trackpad;

import androidx.appcompat.app.AppCompatActivity;

import android.content.Context;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.graphics.Rect;
import android.os.Bundle;
import android.util.Log;
import android.util.SparseArray;
import android.view.MotionEvent;
import android.view.VelocityTracker;
import android.view.View;

import java.util.HashSet;

// reference: https://developer.android.com/training/gestures/movement

public class TrackpadActivity extends AppCompatActivity
{
    private View myView;
    private final String logTag = "TrackpadManager";
    private final int MAX_POINTERS = 2; // only recognize at most 2 fingers

    @Override
    protected void onCreate(Bundle savedInstanceState)
    {
        super.onCreate(savedInstanceState);
        hideSystemUI();
        myView = new TouchScreenView(this);
        setContentView(myView);
    }

    // this class is from:: https://stackoverflow.com/questions/17826358/drag-and-move-a-circle-drawn-on-canvas
    public class TouchScreenView extends View
    {
        // a single circle
        private class SingleCircle
        {
            public final int radius = 50;
            public int centerX;
            public int centerY;

            SingleCircle(int centerX, int centerY)
            {
                this.centerX = centerX;
                this.centerY = centerY;
            }
        }

        private final Paint paint = new Paint(Paint.ANTI_ALIAS_FLAG);
        private Rect screen_size = new Rect(0,0,0,0);
        private HashSet<SingleCircle> myCircles = new HashSet<>(MAX_POINTERS);
        private SparseArray<SingleCircle> myCirclePointers = new SparseArray<>(MAX_POINTERS);

        private VelocityTracker myVelocityTracker = null;

        // set variables to identify gestures
        private long touch_timer = 0; // set timer for click
        private short touch_pointer_count = 0; // count pointers
        private boolean touch_pointer_click = true; // click or move
        private boolean touch_double_clicked = false; // for drag event

        public TouchScreenView(Context context)
        {
            super(context);
            paint.setColor(Color.argb(255, 225, 71, 252));
            paint.setStyle(Paint.Style.FILL);
        }

        @Override
        protected void onDraw(Canvas canvas) {
            // set background color
            canvas.drawColor(Color.WHITE);
            for(SingleCircle circle : myCircles)
            {
                canvas.drawCircle(circle.centerX, circle.centerY, circle.radius, paint);
            }
            postInvalidateOnAnimation();
        }

        @Override
        public boolean onTouchEvent(MotionEvent event)
        {
            boolean handled = false;

            int index = event.getActionIndex();
            int action = event.getActionMasked();
            int pointerID;

            SingleCircle newCircle;
            int posX;
            int posY;

            switch(action)
            {
                case MotionEvent.ACTION_DOWN:
                    Log.d(logTag, "onTouchEvent: down");

                    // first finger down, clear previous pointers
                    myCirclePointers.clear();
                    posX = (int)event.getX(index);
                    posY = (int)event.getY(index);
                    newCircle = getTouchedCircle(posX, posY);
                    newCircle.centerX = posX;
                    newCircle.centerY = posY;
                    myCirclePointers.put(event.getPointerId(index), newCircle);
                    invalidate();

                    // init velocity tracker
                    if(myVelocityTracker == null)
                    {
                        // retrieve a new VelocityTracker
                        myVelocityTracker = VelocityTracker.obtain();
                    }
                    else
                    {
                        // reset velocity tracker
                        myVelocityTracker.clear();
                    }
                    myVelocityTracker.addMovement(event);

                    // update gesture information
                    if((touch_timer != 0) && (System.currentTimeMillis() - touch_timer <= 50))
                        touch_double_clicked = true;
                    touch_timer = System.currentTimeMillis();
                    touch_pointer_count += 1;

                    handled = true;
                    break;
                case MotionEvent.ACTION_POINTER_DOWN:
                    Log.d(logTag, "onTouchEvent: pointer down");

                    // this case, it will be secondary pointers
                    if(event.getPointerCount() > MAX_POINTERS)
                        break; // if already MAX_POINTERS, then ignore new pointer
                    pointerID = event.getPointerId(index);
                    posX = (int)event.getX(index);
                    posY = (int)event.getY(index);
                    newCircle = getTouchedCircle(posX, posY);
                    newCircle.centerX = posX;
                    newCircle.centerY = posY;
                    myCirclePointers.put(pointerID, newCircle);
                    invalidate();

                    // update gesture information
                    touch_pointer_count += 1;

                    handled = true;
                    break;
                case MotionEvent.ACTION_MOVE:
                    Log.d(logTag, "onTouchEvent: move");

                    // update position for each pointer
                    int count = event.getPointerCount();
                    for(index = 0; index < count; index++)
                    {
                        pointerID = event.getPointerId(index);
                        posX = (int)event.getX(index);
                        posY = (int)event.getY(index);
                        newCircle = myCirclePointers.get(pointerID);
                        if(newCircle != null)
                        {
                            newCircle.centerX = posX;
                            newCircle.centerY = posY;
                        }
                    }
                    invalidate();

                    // update velocity tracker
                    myVelocityTracker.addMovement(event);
                    myVelocityTracker.computeCurrentVelocity(8);
                    pointerID = event.getPointerId(0); // only use the velocity of the first finger
                    float velX = myVelocityTracker.getXVelocity(pointerID);
                    float velY = myVelocityTracker.getYVelocity(pointerID);
                    Log.d(logTag, "onTouchEvent: (" + velX + "," + velY + ")");

                    // update gesture information
                    if((Math.abs(velX) > 1.0f) || (Math.abs(velY) > 1.0f) || !touch_pointer_click)
                    {
                        touch_pointer_click = false;
                        if(touch_pointer_count > 1)
                        {
                            // scroll event
                            if(Math.abs(velX) >= Math.abs(velY))
                            {
                                // horizontal scroll
                                MainActivity.addData(new MainActivity.BufferSingle(MainActivity.DATA_TYPE.DATA_TYPE_SCROLL_HORI, velX, 0));
                                Log.d(logTag, "onTouchEvent: scroll horizontal");
                            }
                            else
                            {
                                // vertical scroll
                                MainActivity.addData(new MainActivity.BufferSingle(MainActivity.DATA_TYPE.DATA_TYPE_SCROLL_VERT, 0, velY));
                                Log.d(logTag, "onTouchEvent: scroll vertical");
                            }
                        }
                        else
                        {
                            if(touch_double_clicked)
                            {
                                // drag event
                                MainActivity.addData(new MainActivity.BufferSingle(MainActivity.DATA_TYPE.DATA_TYPE_DRAG, velX, velY));
                                Log.d(logTag, "onTouchEvent: drag");
                            }
                            else
                            {
                                // move event
                                MainActivity.addData(new MainActivity.BufferSingle(MainActivity.DATA_TYPE.DATA_TYPE_MOVE, velX, velY));
                                Log.d(logTag, "onTouchEvent: move");
                            }
                        }
                    }

                    handled = true;
                    break;
                case MotionEvent.ACTION_POINTER_UP:
                    Log.d(logTag, "onTouchEvent: pointer up");

                    // remove pointer when secondary finger is up
                    pointerID = event.getPointerId(index);
                    if(myCirclePointers.get(pointerID, null) != null)
                    {
                        myCirclePointers.remove(pointerID);
                        posX = (int)event.getX(index);
                        posY = (int)event.getY(index);
                        newCircle = getTouchedCircle(posX, posY);
                        myCircles.remove(newCircle);
                        invalidate();
                    }

                    handled = true;
                    break;
                case MotionEvent.ACTION_UP:
                    Log.d(logTag, "onTouchEvent: up");

                    // clear circle pointers when all fingers are up
                    myCirclePointers.clear();
                    myCircles.clear();

                    // prepare data pack
                    if(touch_pointer_click || (System.currentTimeMillis() - touch_timer <= 50))
                    {
                        // this is a click event
                        if(touch_pointer_count == 1)
                        {
                            // left click
                            MainActivity.addData(new MainActivity.BufferSingle(MainActivity.DATA_TYPE.DATA_TYPE_CLICK_LEFT, 0, 0));
                            Log.d(logTag, "onTouchEvent: left click");
                        }
                        else if(touch_pointer_count == 2)
                        {
                            // right click
                            MainActivity.addData(new MainActivity.BufferSingle(MainActivity.DATA_TYPE.DATA_TYPE_CLICK_RIGHT, 0, 0));
                            Log.d(logTag, "onTouchEvent: right click");
                        }
                    }

                    // reset gesture information
                    touch_timer = System.currentTimeMillis();
                    touch_pointer_count = 0;
                    touch_pointer_click = true;
                    touch_double_clicked = false;

                    handled = true;
                    break;
                case MotionEvent.ACTION_CANCEL:
                    Log.d(logTag, "onTouchEvent: cancel");

                    // release velocity tracker
                    myVelocityTracker.recycle();

                    // reset gesture information
                    touch_timer = System.currentTimeMillis();
                    touch_pointer_count = 0;
                    touch_pointer_click = true;
                    touch_double_clicked = false;

                    handled = true;
                    break;
            }
            return super.onTouchEvent(event) || handled;
        }

        @Override
        protected void onMeasure(int widthMeasureSpec, int heightMeasureSpec)
        {
            super.onMeasure(widthMeasureSpec, heightMeasureSpec);
            screen_size.set(0, 0, getMeasuredWidth(), getMeasuredHeight());
        }

        // helper function to retrieve touched circle
        private SingleCircle getTouchedCircle(int posX, int posY)
        {
            SingleCircle touched = null;
            for(SingleCircle circle : myCircles)
            {
                // compute distance
                if((Math.pow(circle.centerX - posX, 2) + Math.pow(circle.centerY - posY, 2)) <= Math.pow(circle.radius, 2))
                {
                    // if already inside a saved touched circle, then use it
                    touched = circle;
                    break;
                }
            }
            if(touched == null)
            {
                // if not found in touched circles, then create a new one
                touched = new SingleCircle(posX, posY);
                if(myCircles.size() == MAX_POINTERS)
                {
                    myCircles.clear();
                }
                myCircles.add(touched);
            }
            return touched;
        }
    }

    // hide system UI when window is focused
    @Override
    public void onWindowFocusChanged(boolean hasFocus)
    {
        super.onWindowFocusChanged(hasFocus);
        if (hasFocus)
            hideSystemUI();
    }

    // hide system UI
    private void hideSystemUI()
    {
        View decorView = getWindow().getDecorView();
        decorView.setSystemUiVisibility(View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY
                | View.SYSTEM_UI_FLAG_LAYOUT_STABLE
                | View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION
                | View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN
                | View.SYSTEM_UI_FLAG_HIDE_NAVIGATION
                | View.SYSTEM_UI_FLAG_FULLSCREEN);
    }
}