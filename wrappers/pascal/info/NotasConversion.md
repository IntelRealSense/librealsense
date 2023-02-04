# Notas sobre la conversión

La libreria es declarada como externa y el tipo de llamada cdecl.

El nombre de la librería externa es realsense2. Se declara en el archivo rs_type.pas  como una constante en función del sistema operativo a compilar:

    const
    {$IFDEF MSWINDOWS}
      REALSENSE_DLL='realsense2.dll';
    {$ENDIF}
    {$IFDEF LINUX}
      REALSENSE_DLL='realsense2.so';
    {$ENDIF}   


## Funciones void

Las funciones void (que no retornan nada).

    void rs2_delete_context(rs2_context* context);

Se declaran en pascal como procedimientos (procedure)

    procedure rs2_delete_context(context: prs2_context); cdecl; external REALSENSE_DLL;    

## Funciones que retornan un puntero

Una función que retorna un punto como la siguiente:

    rs2_context* rs2_create_context(int api_version, rs2_error** error);

Es declarada como una función que devuelve un puntero. 

    type
      rs2_context = record
    end;
    prs2_context = ^rs2_context;    
      (...)
    function rs2_create_context(api_version: integer; aerror: Pointer): prs2_context;cdecl; external REALSENSE_DLL;

El puntero debe apuntar a un tipo de dato o estructura conocida. Pero hay veces que no es posible como la definición anterior.

## Cuando la función retorna un estructura desconocida u opaca

Algunas declaraciones en C devuelve una estructura que no es conocida, o puede ser diferente dependiendo de la situación. Por ejemplo:

    typedef struct rs2_context rs2_context;

Esto, en Free Pascal, se denomina una estructura opaca, de acuerdo con el post: https://forum.lazarus.freepascal.org/index.php/topic,61311.0.html

Se declaran así:

    type
      rs2_context = record
    end;
    prs2_context = ^rs2_context; 

Otra opción es usar un punto opaco. 
    
    Type
     prs2_context=POpaqueData;

Más información en la ayuda de Free Pascal:
* https://www.freepascal.org/docs-html/rtl/system/popaquedata.html
* https://www.freepascal.org/docs-html/rtl/system/topaquedata.html

## Cuando una función es un callback

La siguiente declaración es un callback en C.

    typedef struct rs2_devices_changed_callback rs2_devices_changed_callback;
    
    typedef void (*rs2_devices_changed_callback_ptr)(rs2_device_list*, rs2_device_list*, void*);

En Pascal se debe definir de la siguiente manera:

    type 
      pRS2_devices_changed_callback = ^RS2_devices_changed_callback;

      RS2_devices_changed_callback = procedure (DeviceList1: pRS2_Device_List; 
                                     DeviceList2: pRS2_Device_List);cdecl;
                                     
## Conversión de tipos de datos

|             C              |Pascal                         |  Bits
|----------------------------|-------------------------------|--------------|
| char                       | char                          | 8 bits(ascci)
| signed char                | shortint                      | 8 bits con signo
| unsigned char              | byte                          | 8 bits sin signo
| char                       | pchar                         | 32 bit (puntero a null-terminated string)
| short int                  | smalint                       | 16 bits con signo
| unsigned short int         | word                          | 16 sin signo
| int                        | integer                       | 16 (o 32) bits (genérico) con signo
| unsigned int               | cardinal                      | 16 (o 32) bits (genérico) sin signo
| float                      | single                        | 32 bits con signo
| long long int              | Int64                         | 64 bits con signo
| double                     | double                        | 64 bits con signo
| unsigned long long int     | uInt64                        | 64 sin signo

## Estructura con una rutina en la declaración

Por ejemplo:

    typedef struct rs2_software_video_frame
    {
    void* pixels;
    void(*deleter)(void*);
    int stride;
    int bpp;
    rs2_time_t timestamp;
    rs2_timestamp_domain domain;
    int frame_number;
    const rs2_stream_profile* profile;
    float depth_units;
    } rs2_software_video_frame;

La línea:

    void(*deleter)(void*);

type
    
    TDeleterProc = procedure(arg: Pointer); cdecl;

    type
      Trs2_software_video_frame = record
      pixels: pvoid;
      deleter: TDeleterProc;
      stride: integer;
      timestamp: Trs2_time_t;
      domain: Trs2_timestamp_domain;
      frame_number: integer;
      profile: pRS2_stream_profile;
      depth_units: single;
    end; 

 
Preguntado en el foro:
https://forum.lazarus.freepascal.org/index.php/topic,61311.0.html?PHPSESSID=tajqr2m9bi89t347sl80bapj70 }

                                                   









