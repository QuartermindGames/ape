# MDL v3 Format Specification

Everything is a required field unless stated otherwise.

```text
object model
{
    uint16 version 3
    string name "" ; optional
    string author "" ; optional
    uint32 timestamp 0 ; optional
    
    array object vertices
    {
        {
            array float position { 0 0 0 }
            array float normal { 0 0 0 }    ; optional
            array float uv { 0 0 }          ; optional
            array object weights
            {
                uint32 boneIndex 0
                float weight 0
            } ; optional
        }
    }
    
    array object meshes
    {
        {
            string material "" 
            array uint32 indices 
            {
                0 0 0 ; triangle
            }
        }
    }
    
    array object bones
    {
        {
            string name ""
            uint32 parentIndex
            array float translate { 0 0 0 }
            array float rotate { 0 0 0 }
            array float scale { 0 0 0 }
        }
    }
}
```
