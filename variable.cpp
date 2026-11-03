#include <Windows.h>
#include "variable.h"

// 전역 변수 정의 공간

//격자판(보드)
int gridSpacing = 40; // 격자의 간격
RECT clientRect{}; // 클라이언트 영역

//게임 시작 전
bool gamestart = false;
bool stageManager = false; // 스테이지 선택 화면

// 주인공 원
double playerX, playerY; // 주인공 좌표
int playerSize = 10; // 플레이어 반지름
double playerSpeed = 3; // 플레이어 속도
int moveTimerSpeed = 15; // 타이머 속도
double playerJumpStartY; // 주인공이 점프한 Y 좌표
double playerJumpHeight; // 주인공이 점프한 높이
RECT playerRT;
bool doubleJumpMode = false; // 플레이어 더블 점프 아이템 먹음

// 주인공 이동 bool
bool playerDropping = true; // 플레이어 낙하 중
bool playerMoveStart = false; // 플레이어 움직이기 시작했는지
bool RightMoving = false; // 오른쪽으로 움직이고 있었는지

//마우스
double mouseX, mouseY; // 마우스 좌표
bool LMBHold = false; // 좌클릭 누르고 있는지
bool RMBHold = false; // 우클릭 누르고 있는지

int stage{}; // 현재 스테이지
HBITMAP menuAni[9]{};
HBITMAP BlockBitmap[7]; // 각 블럭들 비트맵

// 바탕화면 애니메이션
int menuAniNum{};	// 메뉴 바탕화면 애니메이션 비트맵 선택 변수
int aniTimerSpeed = 120; // 타이머 속도

HBITMAP stageBitmap[5]; // 스테이지 그림
RECT stageBitRect[5]; // 스테이지 그림의 영역

Block* firstBlockPointer = nullptr; // 첫 블럭

int BlockCnt{}; // 블럭 총 개수

POINT stageStartPoint; // 스테이지 시작 위치

int SetBlockType{};
bool SetBlockTypeSelect = false; // 설치할 블록 선택했는지
bool playerPointSet = false; // 플레이어 위치 설정 모드
bool blockDeleteMode = false; // 블럭 삭제 모드

int starCnt{}; // 스테이지의 별 남은 개수
bool stageClear = false; // 스테이지 클리어 여부
RECT stageClearBox{}; // 스테이지 클리어 후 나올 박스 영역
HBITMAP retry;// 스테이지 클리어 후 나오는 다시하기 버튼
HBITMAP home; // 스테이지 클리어 후 나오는 홈 버튼
HBITMAP nextStage; // 스테이지 클리어 후 나오는 다음 스테이지 버튼
RECT retryRT; // 다시하기 버튼 영역
RECT homeRT; // 홈 버튼 영역
RECT nextStageRT; // 다음 스테이지 버튼 영역
bool stageCleared[5]{}; // 맵 파일에 저장될 각 스테이지 클리어 기록

//ui button bmp or animation
HBITMAP buttonUI[2]{}; // ui 기본, 눌렸을 때
int buttonNum[2]{}; // 버튼 bmp선택

// 전역 인스턴스 및 폰트
HINSTANCE g_hInst;
HFONT g_hCustomFont = NULL;
HFONT g_hCustomFont1 = NULL;
HFONT g_hCustomFont2 = NULL;