package com.example.draw;

import android.annotation.TargetApi;
import android.content.IOnPrimaryClipChangedListener;
import android.graphics.PixelFormat;
import android.os.Build;
import android.os.RemoteException;
import android.util.Log;
import android.view.Display;
import android.view.IRotationWatcher;
import android.view.Surface;
import android.view.SurfaceControl;

import com.example.draw.wrappers.DisplayManager;
import com.example.draw.wrappers.ServiceManager;
import com.example.draw.wrappers.Size;

import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;

public class Main {

    public static final String TAG = "日志";

    static native void native_surfaceCreate(Surface surface, int width, int height);

    static native void native_surfaceChanged(int rotation);

    @TargetApi(Build.VERSION_CODES.Q)
    public static void main(String[] args) {
        // Close STDOUT/STDERR since it belongs to the parent shell
        /*System.out.close();
        System.err.close();*/

        String classPath = System.getProperty("java.class.path");
        String directory = classPath.substring(0, classPath.lastIndexOf("/"));
        Log.d(TAG, "main: directory = " + directory);

        System.load(directory + "/libDrawCore.so");

        boolean isHide = true;
        boolean isSecure = false;
        int width, height;

        DisplayManager displayManager = ServiceManager.getDisplayManager();
        Size size = displayManager.getDisplayInfo(Display.DEFAULT_DISPLAY).getSize();

        if (size.getWidth() > size.getHeight()) {
            width = size.getWidth();
            height = size.getHeight();
        } else {
            width = size.getHeight();
            height = size.getWidth();
        }
        Log.d(TAG, "Main: width = " + width + ", height = " + height);
        SurfaceControl.Builder builder = new SurfaceControl.Builder();
        builder.setName("enennb");
        builder.setFormat(PixelFormat.RGBA_8888);
        if (Build.VERSION.SDK_INT <= 30) {
            try {
                // 获取 SurfaceControl.Builder 类
                Class<?> builderClass = Class.forName("android.view.SurfaceControl$Builder");
                // 获取 setMetadata 方法
                Method setMetadataMethod = builderClass.getDeclaredMethod("setMetadata", int.class, int.class);
                // 设置可访问性
                setMetadataMethod.setAccessible(true);
                // 调用 setMetadata 方法设置元数据]
                if (isHide && !isSecure)
                    setMetadataMethod.invoke(builder, 2, 441731);
                // 获取 setFlags 方法
                Method setFlagsMethod = builderClass.getDeclaredMethod("setFlags", int.class);
                // 设置可访问性
                setFlagsMethod.setAccessible(true);
                setFlagsMethod.invoke(builder, isSecure ? 0x80 : 0x0);
            } catch (ClassNotFoundException | IllegalAccessException |
                     NoSuchMethodException | InvocationTargetException ignored) {
            }
        } else {
            try {
                // 获取 SurfaceControl.Builder 类
                Class<?> builderClass = Class.forName("android.view.SurfaceControl$Builder");
                // 获取 setFlags 方法
                Method setFlagsMethod = builderClass.getDeclaredMethod("setFlags", int.class);
                // 设置可访问性
                setFlagsMethod.setAccessible(true);
                setFlagsMethod.invoke(builder, isSecure ? 0x80 : isHide ? 0x40 : 0x0);
            } catch (ClassNotFoundException | IllegalAccessException |
                     NoSuchMethodException | InvocationTargetException ignored) {
            }
        }
        int rotation = ServiceManager.getWindowManager().getRotation();
        if (rotation == 1 || rotation == 3) {
            builder.setBufferSize(width, height);
        } else {
            builder.setBufferSize(height, width);
        }
        SurfaceControl surfaceControl = builder.build();

        SurfaceControl.Transaction transaction = new SurfaceControl.Transaction();
        transaction.setLayer(surfaceControl, Integer.MAX_VALUE);
        transaction.apply();
        transaction.close();

        ServiceManager.getWindowManager().registerRotationWatcher(new IRotationWatcher.Stub() {
            @Override
            public void onRotationChanged(int rotation) {
                SurfaceControl.Transaction transaction = new SurfaceControl.Transaction();
                transaction.setLayer(surfaceControl, Integer.MAX_VALUE);
                if (rotation == 1 || rotation == 3) {
                    transaction.setBufferSize(surfaceControl, width, height);
                } else {
                    transaction.setBufferSize(surfaceControl, height, width);
                }
                transaction.apply();
                transaction.close();
                native_surfaceChanged(rotation);
            }
        }, 0);

        Surface surface = new Surface(surfaceControl);
        native_surfaceCreate(surface, width, height);
        surface.release();
        Log.d(TAG, "main: 结束");
    }

    public static String getClipText() {
        return ServiceManager.getClipboardManager().getText().toString();
    }

}
