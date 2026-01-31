using System;
using System.Runtime.CompilerServices;

namespace DabozzEngine
{
    public struct Vector3
    {
        public float X;
        public float Y;
        public float Z;

        public Vector3(float x, float y, float z)
        {
            X = x;
            Y = y;
            Z = z;
        }

        public static Vector3 Zero => new Vector3(0, 0, 0);
        public static Vector3 One => new Vector3(1, 1, 1);
        public static Vector3 Up => new Vector3(0, 1, 0);
        public static Vector3 Down => new Vector3(0, -1, 0);
        public static Vector3 Left => new Vector3(-1, 0, 0);
        public static Vector3 Right => new Vector3(1, 0, 0);
        public static Vector3 Forward => new Vector3(0, 0, 1);
        public static Vector3 Back => new Vector3(0, 0, -1);
    }

    public struct Quaternion
    {
        public float X;
        public float Y;
        public float Z;
        public float W;

        public Quaternion(float x, float y, float z, float w)
        {
            X = x;
            Y = y;
            Z = z;
            W = w;
        }

        public static Quaternion Identity => new Quaternion(0, 0, 0, 1);
    }

    public class Entity
    {
        public uint ID { get; private set; }

        public Entity(uint id)
        {
            ID = id;
        }

        public static Entity Create()
        {
            uint id = Internal_Create();
            return new Entity(id);
        }

        public void Destroy()
        {
            Internal_Destroy(ID);
        }

        public bool IsValid()
        {
            return Internal_IsValid(ID);
        }

        [MethodImpl(MethodImplOptions.InternalCall)]
        private static extern uint Internal_Create();

        [MethodImpl(MethodImplOptions.InternalCall)]
        private static extern void Internal_Destroy(uint entityID);

        [MethodImpl(MethodImplOptions.InternalCall)]
        private static extern bool Internal_IsValid(uint entityID);
    }

    public class Transform
    {
        private uint entityID;

        public Transform(uint entityID)
        {
            this.entityID = entityID;
        }

        public Vector3 Position
        {
            get
            {
                Internal_GetPosition(entityID, out float x, out float y, out float z);
                return new Vector3(x, y, z);
            }
            set
            {
                Internal_SetPosition(entityID, value.X, value.Y, value.Z);
            }
        }

        public Quaternion Rotation
        {
            get
            {
                Internal_GetRotation(entityID, out float x, out float y, out float z, out float w);
                return new Quaternion(x, y, z, w);
            }
            set
            {
                Internal_SetRotation(entityID, value.X, value.Y, value.Z, value.W);
            }
        }

        public Vector3 Scale
        {
            get
            {
                Internal_GetScale(entityID, out float x, out float y, out float z);
                return new Vector3(x, y, z);
            }
            set
            {
                Internal_SetScale(entityID, value.X, value.Y, value.Z);
            }
        }

        [MethodImpl(MethodImplOptions.InternalCall)]
        private static extern void Internal_GetPosition(uint entityID, out float x, out float y, out float z);

        [MethodImpl(MethodImplOptions.InternalCall)]
        private static extern void Internal_SetPosition(uint entityID, float x, float y, float z);

        [MethodImpl(MethodImplOptions.InternalCall)]
        private static extern void Internal_GetRotation(uint entityID, out float x, out float y, out float z, out float w);

        [MethodImpl(MethodImplOptions.InternalCall)]
        private static extern void Internal_SetRotation(uint entityID, float x, float y, float z, float w);

        [MethodImpl(MethodImplOptions.InternalCall)]
        private static extern void Internal_GetScale(uint entityID, out float x, out float y, out float z);

        [MethodImpl(MethodImplOptions.InternalCall)]
        private static extern void Internal_SetScale(uint entityID, float x, float y, float z);
    }

    public static class Input
    {
        public static bool GetKey(int keyCode)
        {
            return Internal_GetKey(keyCode);
        }

        public static bool GetKeyDown(int keyCode)
        {
            return Internal_GetKeyDown(keyCode);
        }

        public static bool GetKeyUp(int keyCode)
        {
            return Internal_GetKeyUp(keyCode);
        }

        public static Vector3 MousePosition
        {
            get
            {
                Internal_GetMousePosition(out float x, out float y);
                return new Vector3(x, y, 0);
            }
        }

        [MethodImpl(MethodImplOptions.InternalCall)]
        private static extern bool Internal_GetKey(int keyCode);

        [MethodImpl(MethodImplOptions.InternalCall)]
        private static extern bool Internal_GetKeyDown(int keyCode);

        [MethodImpl(MethodImplOptions.InternalCall)]
        private static extern bool Internal_GetKeyUp(int keyCode);

        [MethodImpl(MethodImplOptions.InternalCall)]
        private static extern void Internal_GetMousePosition(out float x, out float y);
    }

    public static class Debug
    {
        public static void Log(string message)
        {
            Internal_Log(message);
        }

        [MethodImpl(MethodImplOptions.InternalCall)]
        private static extern void Internal_Log(string message);
    }

    public abstract class ScriptBehaviour
    {
        public uint EntityID { get; internal set; }
        public Transform Transform { get; private set; }

        public ScriptBehaviour()
        {
            Transform = new Transform(EntityID);
        }

        public virtual void OnStart() { }
        public virtual void OnUpdate(float deltaTime) { }
        public virtual void OnDestroy() { }
    }
}
