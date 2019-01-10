/*
 * queue.c
 *
 *  Created on: 2017. 7. 23.
 *      Author: Yummy
 */

#include <queue.h>

// 큐 초기화
void kInitQueue(QUEUE *queue, void *buf, int maxCnt, int size) {
	// 큐 최대 개수와 크기, 버퍼 어드레스 저장
	queue->maxCnt = maxCnt;
	queue->size = size;
	queue->arr = buf;

	// 큐 삽입 및 제거 위치 초기화, 마지막으로 수행된 명령 제거로 설정해 큐를 비움
	queue->putIdx = 0;
	queue->getIdx = 0;
	queue->lastPut = FALSE;
}

// 큐가 가득 찼는지 여부 반환
BOOL kIsQFull(const QUEUE *queue) {
	// 큐 삽입 및 제거 인덱스 같고 마지막으로 수행된 명령이 삽입이면 가득 찬 것이므로 삽입 불가능
	if((queue->getIdx == queue->putIdx) && (queue->lastPut == TRUE)) return TRUE;
	return FALSE;
}

// 큐가 비어있는지 여부 반환
BOOL kIsQEmpty(const QUEUE *queue) {
	// 큐 삽입 및 제거 인덱스 같고 마지막으로 수행된 명령이 제거이면 큐가 비었으니 제거 불가능
	if((queue->getIdx == queue->putIdx) && (queue->lastPut == FALSE)) return TRUE;
	return FALSE;
}

// 큐에 데이터 삽입
BOOL kAddQData(QUEUE *queue, const void *data) {
	// 큐가 가득 찼으면 삽입 불가능
	if(kIsQFull(queue) == TRUE) return FALSE;

	// 삽입 인덱스가 가리키는 위치에서 데이터 크기만큼 복사
	kMemCpy((char*)queue->arr + (queue->size * queue->putIdx), data, queue->size);

	// 삽입 인덱스 변경 및 삽입 동작 수행 기록
	queue->putIdx = (queue->putIdx + 1) % queue->maxCnt;
	queue->lastPut = TRUE;
	return TRUE;
}

// 큐에서 데이터 제거
BOOL kRmQData(QUEUE *queue, void *data) {
	// 큐가 비었으면 제거 불가능
	if(kIsQEmpty(queue) == TRUE) return FALSE;

	// 제거 인덱스가 가리키는 위치에서 데이터 크기만큼 복사
	kMemCpy(data, (char*)queue->arr + (queue->size * queue->getIdx), queue->size);

	// 제거 인덱스 변경 및 제거 동작 수행 기록
	queue->getIdx = (queue->getIdx + 1) % queue->maxCnt;
	queue->lastPut = FALSE;
	return TRUE;
}
