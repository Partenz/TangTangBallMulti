#pragma once
#include <Windows.h>
// 전역 변수 선언 공간

// 블럭 구조체
struct Block {
	RECT rt{}; // 블럭 영역
	Block* prev{}; // 이전 블럭
	Block* next{}; // 다음 블럭
	int BlockType{}; // 블럭 종류
};

#define BlockType_Basic 0
#define BlockType_Thorn 1
#define BlockType_Once 2
#define BlockType_Jump 3
#define BlockType_Star 4
#define BlockType_DoubleJump 5
#define BlockType_UpDown 6

//격자판(보드)
extern int gridSpacing; // 격자의 간격
extern RECT clientRect; // 클라이언트 영역

//게임 시작 전
extern bool gamestart;
extern bool stageManager; // 스테이지 선택 화면


// 주인공 원
extern double playerX, playerY; // 주인공 좌표
extern int playerSize; // 플레이어 반지름
extern double playerSpeed; // 플레이어 속도
extern int moveTimerSpeed; // 타이머 속도
extern double playerJumpStartY; // 주인공이 점프한 Y 좌표
extern double playerJumpHeight; // 주인공이 점프한 높이
extern RECT playerRT; // 플레이어 히트박스
extern bool doubleJumpMode; // 플레이어 더블 점프 아이템 먹음

// 주인공 이동 bool
extern bool playerDropping; // 플레이어 낙하 중
extern bool playerMoveStart; // 플레이어 움직이기 시작했는지
extern bool RightMoving; // 오른쪽으로 움직이고 있었는지

//마우스
extern double mouseX, mouseY; // 마우스 좌표
extern bool LMBHold; // 좌클릭 누르고 있는지
extern bool RMBHold; // 우클릭 누르고 있는지

extern int stage; // 현재 스테이지
extern HBITMAP menuAni[9];
extern HBITMAP BlockBitmap[7]; // 각 블럭들 비트맵

// 바탕화면 애니메이션
extern int menuAniNum;	// 메뉴 바탕화면 애니메이션 비트맵 선택 변수
extern int aniTimerSpeed; // 타이머 속도

extern HBITMAP stageBitmap[5]; // 스테이지 그림
extern RECT stageBitRect[5]; // 스테이지 그림의 영역

extern int BlockCnt; // 블럭 총 개수
extern Block* firstBlockPointer; // 첫 블럭
extern POINT stageStartPoint; // 스테이지 시작 위치

extern int SetBlockType; // 설치할 블록 종류
extern bool SetBlockTypeSelect; // 설치할 블록 선택했는지
extern bool playerPointSet; // 플레이어 위치 설정 모드
extern bool blockDeleteMode; // 블럭 삭제 모드

extern int starCnt; // 스테이지의 별 남은 개수
extern bool stageClear; // 스테이지 클리어 여부
extern RECT stageClearBox; // 스테이지 클리어 후 나올 박스 영역
extern HBITMAP retry;// 스테이지 클리어 후 나오는 다시하기 버튼
extern HBITMAP home; // 스테이지 클리어 후 나오는 홈 버튼
extern HBITMAP nextStage; // 스테이지 클리어 후 나오는 다음 스테이지 버튼
extern RECT retryRT; // 다시하기 버튼 영역
extern RECT homeRT; // 홈 버튼 영역
extern RECT nextStageRT; // 다음 스테이지 버튼 영역
extern bool stageCleared[5]; // 맵 파일에 저장될 각 스테이지 클리어 기록

//ui button bmp or animation
extern HBITMAP buttonUI[2]; // ui 기본, 눌렸을 때
extern int buttonNum[2]; // 버튼 bmp선택