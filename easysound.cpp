#include <easysound.h>
#include <direct.h>
#include "wave.h"

using namespace std;

EASYSOUND::EASYSOUND(HWND hwnd)
{
	DSBUFFERDESC Descriptor;
	WAVEFORMATEX WaveFormat;
	LPDIRECTSOUNDBUFFER primarybuffer;

	WindowHandle = hwnd;
	BufferCount = 0;
	if( FAILED(DirectSoundCreate(NULL, &SoundObject, NULL)) )
	{
		ErrorMessage("error initializing directsound");
		return;
	}

	if( FAILED(SoundObject->SetCooperativeLevel(WindowHandle, DSSCL_PRIORITY)) )
	{
		ErrorMessage("error in setcooperativelevel");
		return;
	}

	INIT_STRUCT(Descriptor);
	Descriptor.dwBufferBytes = 0; // must be 0 for primary buffer
	Descriptor.dwFlags = DSBCAPS_PRIMARYBUFFER;
	Descriptor.lpwfxFormat = NULL; // must be NULL for primary buffer

	memset(&WaveFormat, 0, sizeof(WaveFormat));
	WaveFormat.cbSize = sizeof(WaveFormat);
	WaveFormat.wFormatTag = WAVE_FORMAT_PCM;
	WaveFormat.wBitsPerSample = 16;
	WaveFormat.nChannels = 2;
	WaveFormat.nSamplesPerSec = 44100;
	WaveFormat.nBlockAlign = WaveFormat.wBitsPerSample / 8 * WaveFormat.nChannels;
	WaveFormat.nAvgBytesPerSec = WaveFormat.nSamplesPerSec * WaveFormat.nBlockAlign;

	if( SUCCEEDED(SoundObject->CreateSoundBuffer(&Descriptor, &primarybuffer, NULL)) )
		primarybuffer->SetFormat(&WaveFormat);

	for (int i = 0; i < MAXBUFFERS; i++)
	{
		sounds[i].buffer = NULL;
		SoundList.push(i);
	}
}

EASYSOUND::~EASYSOUND()
{
	for( int i = 0; i < MAXBUFFERS; i++ )
	{
		if( sounds[i].buffer != NULL )
			sounds[i].buffer->Release();
	}
}

SoundID EASYSOUND::LoadSound2(char *file)
{
	DWORD dwNumRead, dwLength;
	char buffer[5];
	buffer[4] = '\0';

	SoundID index = FirstEmptySound();
	if( index == -1 )
		return index;

	HANDLE hFile = CreateFile(file, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING,
							 FILE_ATTRIBUTE_NORMAL, 0);
	if( hFile == INVALID_HANDLE_VALUE )
		return -1;
	ReadFile(hFile, buffer, 4, &dwNumRead, 0); // must be "RIFF"
	if( strcmp(buffer, "RIFF") != 0 )
	{
		CloseHandle(hFile);
		return -1;
	}
	ReadFile(hFile, &dwLength, sizeof dwLength, &dwNumRead, 0); // length of file
	SetFilePointer(hFile, 4, 0, FILE_CURRENT); // skip this, contains "WAVE"

	bool done=false;
	UCHAR *ucTemp;
	while( !done )
	{
		ReadFile(hFile, buffer, 4, &dwNumRead, 0); // chunk header
		ReadFile(hFile, &dwLength, sizeof dwLength, &dwNumRead, 0); // length
		ucTemp = new UCHAR[dwLength];
		ReadFile(hFile, ucTemp, dwLength, &dwNumRead, 0);

#pragma message(__FILE__ " CONTINUE IN THIS FILE")
	}

	return index;
}

SoundID EASYSOUND::LoadSound(char *file)
{
	DSBUFFERDESC Descriptor;
	LPVOID lpvAudio1;
	DWORD dwBytes1;
	UINT cbBytesRead;

	SoundID index = FirstEmptySound(); // the index in the sounds array in which the sound will be sstored

	if( index == -1 )
		return index;

	if( WaveOpenFile(file, &sounds[index].hmmio, &sounds[index].pwfx, 
					 &sounds[index].mmckInfoParent) != 0 )
	{
		ErrorMessage("error in waveopenfile");
		return -1;
	}

	if( WaveStartDataRead(&sounds[index].hmmio, &sounds[index].mmckinfo, 
						  &sounds[index].mmckInfoParent) != 0 )
	{
		ErrorMessage("error in wavestartdataread");
		return -1;
	}

	INIT_STRUCT(Descriptor);
	Descriptor.dwFlags = DSBCAPS_STATIC;
	Descriptor.dwBufferBytes = sounds[index].mmckinfo.cksize;
	Descriptor.lpwfxFormat = sounds[index].pwfx;

	if( FAILED(SoundObject->CreateSoundBuffer(&Descriptor, &sounds[index].buffer, NULL)) )
	{
		WaveCloseReadFile(&sounds[index].hmmio, &sounds[index].pwfx);
		ErrorMessage("error in createsoundbuffer");
		return -1;
	}

	if( FAILED(sounds[index].buffer->Lock(0, // lock start offset
										0, // size of lock; ignored
										&lpvAudio1, // address of lock start
										&dwBytes1, // number of bytes locked
										NULL, // no wraparound start
										NULL, // number of wraparound bytes (none)
										DSBLOCK_ENTIREBUFFER)) )
	{
		ErrorMessage("error in lock");
		return -1;
	}

	if( WaveReadFile(sounds[index].hmmio, dwBytes1, (BYTE *)lpvAudio1, &sounds[index].mmckinfo, &cbBytesRead) )
	{
		ErrorMessage("error in wavereadfile");
		return -1;
	}

	sounds[index].buffer->Unlock(lpvAudio1, dwBytes1, NULL, 0);
	BufferCount++;

	return index; // the user should store this value to be able to play the sound back
}

SoundID EASYSOUND::LoadStreamingSound(char *file)
{
	DSBUFFERDESC dsbdesc = {0};
	dsbdesc.dwSize = sizeof dsbdesc;
	
	SoundID index = FirstEmptySound();
    // Open the file, get wave format, and descend to the data chunk.
  if (WaveOpenFile(file, &sounds[index].hmmio, &sounds[index].pwfx, 
			&sounds[index].mmckInfoParent) != 0)
		return FALSE;
  if (WaveStartDataRead(&sounds[index].hmmio, &sounds[index].mmckinfo, &sounds[index].mmckInfoParent) != 0)
		return FALSE;

    // Create secondary buffer able to hold 2 seconds of data.
  dsbdesc.dwFlags = DSBCAPS_GETCURRENTPOSITION2 | DSBCAPS_GLOBALFOCUS | DSBCAPS_CTRLPAN; 
  dsbdesc.dwBufferBytes = sounds[index].pwfx->nAvgBytesPerSec * 2;
  dsbdesc.lpwfxFormat = sounds[index].pwfx; // pointer to waveformatex struct
 
  if( FAILED(SoundObject->CreateSoundBuffer( &dsbdesc, &sounds[index].buffer, NULL)) )
  {
		WaveCloseReadFile(&sounds[index].hmmio, &sounds[index].pwfx);
    return FALSE; 
  }

  FillBufferWithSilence(sounds[index].buffer);
  sounds[index].buffer->Play( 0, 0, DSBPLAY_LOOPING );

  sounds[index].dwMidBuffer = dsbdesc.dwBufferBytes / 2;

	return index;
}

void EASYSOUND::PlayStreamBuffer(SoundID which)
{
	static DWORD    dwLastPlayPos;
    HRESULT         hr;
    DWORD           dwPlay;
    DWORD           dwStartOfs;    
    VOID            *lpvData;
    DWORD           dwBytesLocked;
    UINT            cbBytesRead;

	if( FAILED(sounds[which].buffer->GetCurrentPosition(&dwPlay, NULL)) )
		return;

	
  if (((dwPlay >= sounds[which].dwMidBuffer) && (dwLastPlayPos < sounds[which].dwMidBuffer))
       || (dwPlay < dwLastPlayPos) )
	{		
		dwStartOfs = (dwPlay >= sounds[which].dwMidBuffer) ? 0 : sounds[which].dwMidBuffer;

    hr = sounds[which].buffer->Lock(dwStartOfs, // Offset of lock start.
																		sounds[which].dwMidBuffer, // Number of bytes to lock.
																		&lpvData, // Address of lock start.
																		&dwBytesLocked, // Number of bytes locked.
																		NULL,   // Address of wraparound lock.
																		NULL,   // Number of wraparound bytes.
																		0 );    // Flags.

		WaveReadFile(sounds[which].hmmio,     // File handle.
								 dwBytesLocked,			  // Number of bytes to read.
								 (BYTE *)lpvData,		  // Destination.
								 &sounds[which].mmckinfo, // File chunk info.
								 &cbBytesRead );		  // Number of bytes read.

		if( cbBytesRead < dwBytesLocked )
		{
			if (WaveStartDataRead(&sounds[which].hmmio, &sounds[which].mmckinfo, 
														&sounds[which].mmckInfoParent ) == 0)
      {
				WaveReadFile(sounds[which].hmmio,
										 dwBytesLocked - cbBytesRead,
                     (BYTE *)lpvData + cbBytesRead, 
                     &sounds[which].mmckinfo,      
                     &cbBytesRead );    
      }
		}
	}
}

void EASYSOUND::Play(SoundID which)
{
	if (which < MAXBUFFERS && which >= 0 && sounds[which].buffer != NULL)
	{
		sounds[which].buffer->SetCurrentPosition(0);
		sounds[which].buffer->Play(0, 0, 0);
	}
}

void EASYSOUND::Stop(SoundID which)
{
	if (which < MAXBUFFERS && which >= 0 && sounds[which].buffer != NULL)
		sounds[which].buffer->Stop();
}

SoundID EASYSOUND::FirstEmptySound()
{
	if( SoundList.empty() )
		return -1;	

	int res = SoundList.top();
	SoundList.pop();

	return res;
}

void EASYSOUND::ReleaseSound(SoundID which)
{
	if( which < 0 || which >= MAXBUFFERS || sounds[which].buffer == NULL )
		return;

	sounds[which].buffer->Release();
	sounds[which].buffer = NULL;
	SoundList.push(which);
	BufferCount--;
}

void EASYSOUND::ErrorMessage(char *s)
{
	MessageBox(WindowHandle, s, "directsound error", MB_OK | MB_ICONEXCLAMATION);
}

void EASYSOUND::SetPan(SoundID which, long pan)
{
	if (sounds[which].buffer->SetPan(pan) == DSERR_CONTROLUNAVAIL)
		OutputDebugString("Pan control not supported\n");
}

void EASYSOUND::SetVolume(SoundID which, long volume)
{
	if( DSBVOLUME_MIN < volume && volume < DSBVOLUME_MAX )
	{
		if( sounds[which].buffer->SetVolume(volume) == DSERR_CONTROLUNAVAIL )
			OutputDebugString("Volume control is not supported\n");
	}
}

LPDIRECTSOUNDBUFFER EASYSOUND::GetBuffer(SoundID which)
{
	return sounds[which].buffer;
}

LPDIRECTSOUND EASYSOUND::GetSoundObject()
{
	return SoundObject;
}

BOOL EASYSOUND::FillBufferWithSilence(LPDIRECTSOUNDBUFFER lpDsb)
{
  WAVEFORMATEX wfx;
  DWORD        dwSizeWritten;

  PBYTE pb1;
  DWORD cb1;

  if (FAILED(lpDsb->GetFormat(&wfx, sizeof(WAVEFORMATEX), &dwSizeWritten)))
		return FALSE;

  if (SUCCEEDED(lpDsb->Lock(0, 0, (LPVOID *)&pb1, &cb1, 
                        NULL, NULL, DSBLOCK_ENTIREBUFFER)))
  {
		FillMemory(pb1, cb1, (wfx.wBitsPerSample == 8) ? 128 : 0);
    lpDsb->Unlock(pb1, cb1, NULL, 0);
    return TRUE;
  }

  return FALSE;
}  // FillBufferWithSilence

/*******************************************************************************
MidiFile class
*********************************************************************************/

Midi::Midi(LPDIRECTSOUND lpds)
{
	CoInitialize(NULL);

	// Create the DirectMusic COM Performance Object
	if( FAILED(CoCreateInstance(CLSID_DirectMusicPerformance, NULL, 
								CLSCTX_INPROC, IID_IDirectMusicPerformance2, 
								(void **)&Performance)) )
	{
		ErrorMessage("couldn't create performance object");
		return;
	}

	// initialize the system. lpds can be NULL, then init will create a new DirectSound Object
	if( FAILED(Performance->Init(NULL, lpds, NULL)) )
	{
		ErrorMessage("Performance init failed");
		return;
	}

	if( FAILED(Performance->AddPort(NULL)) )
	{
		ErrorMessage("AddPort failed");
		return;
	}

	// the DirectMusicLoader is also a COM Object. Create it to load midi's
	if( FAILED(CoCreateInstance(CLSID_DirectMusicLoader, NULL,
								CLSCTX_INPROC, IID_IDirectMusicLoader,
								(void **)&Loader)) )
	{
		ErrorMessage("CoCreateInstance Loader failed");
		return;
	}
}

Midi::~Midi()
{
	Performance->Stop(NULL, NULL, 0, 0); // first stop playing
	Segment->SetParam(GUID_Unload, -1, 0, 0, (void *)Performance); // unload instruments
	Segment->Release();
	Performance->CloseDown();
	Performance->Release();
	Loader->Release();
	CoUninitialize();
}

void Midi::Play()
{
	Performance->PlaySegment(Segment, 0, 0, &State);
}

void Midi::Stop()
{
	Performance->Stop(NULL, NULL, 0, 0);
}

bool Midi::LoadMidi(char *file)
{
	DMUS_OBJECTDESC ObjDesc; // DirectMusic Object Descriptor
	
	char szDir[MAX_PATH];
	WCHAR wszDir[MAX_PATH];

	// turn file into a Unicode String. 
	// this must be done because DMUS_OBJECTDESC only handles unicode
	WCHAR wfile[MAX_PATH]; 
	MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, file, -1, wfile, MAX_PATH);

	if( getcwd(szDir, MAX_PATH) == NULL ) // returns UTF-8
		return false;

	// but must be UTF-16
	MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, szDir, -1, wszDir, MAX_PATH);
	if( FAILED(Loader->SetSearchDirectory(GUID_DirectMusicAllTypes, wszDir, NULL)) )
	{
		ErrorMessage("SetSearchDirectory Failed");
		return false;
	}
	
	ObjDesc.guidClass = CLSID_DirectMusicSegment;
	ObjDesc.dwSize = sizeof(ObjDesc);
	wcscpy(ObjDesc.wszFileName, wfile);
	ObjDesc.dwValidData = DMUS_OBJ_CLASS | DMUS_OBJ_FILENAME;

	// Load the mididata contained as described in ObjDesc to the segment
	if( FAILED(Loader->GetObject(&ObjDesc, IID_IDirectMusicSegment2, (void **)&Segment)) )
		ErrorMessage("couldn't load midi data into segment");

	if( Segment )
	{
		// we want a normal 
		Segment->SetParam(GUID_StandardMIDIFile, -1, 0, 0, (void *)Performance);
		// download the instrument data to the card
		Segment->SetParam(GUID_Download, -1, 0, 0, (void *)Performance);
		Segment->SetRepeats(100000); // is there an "endless" macro?
	}
	else
		return false;

	return true;
}

void Midi::ErrorMessage(char *s)
{
	OutputDebugString(s);
	OutputDebugString("\n");
	//MessageBox(GetDesktopWindow(), s, "error", MB_OK);
}