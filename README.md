# 딥러닝 기반 교통 안전 카메라 시스템
이미지 인식 딥러닝 알고리즘 중 하나인 YOLO(You Only Look Once) 알고리즘을 사용하여 도로에서 쓰러진 사람과 교통에 방해 되는 요소 중 하나인 동물 사체를 인식하여 관련 기관에 알려주는 시스템이다.(이 프로젝트에서는 관련 기관에 알리는 기능을 라즈베리파이에서 LED가 켜지는 것으로 대체하였다.)

## 개발 기간 및 환경
- 개발 기간: 2018.03 ~ 2018.06
- YOLO 알고리즘을 사용한 PC 성능
  - CPU: intel core i5-6600
  - RAM: 16GB
  - Graphic card: GTX 1060 6GB
- OS: Linux ubuntu 16.04
- Language: C
- Open source: YOLO, OpenCv(2.4.13.2), CUDA 8.0, cuDNN 6.0, MySQL

## 시스템 구성도

![systemstructure](https://user-images.githubusercontent.com/34755287/51419844-00614400-1bd1-11e9-8d28-b31cf624dd59.JPG)

1. 카메라에서 쓰러진 사람이나 동물 사체를 인식하면 메인 서버에 인식된 이미지 데이터, 카메라 id, 인식된 종류를 전송한다.
2. 카메라에서 받은 데이터를 이용해 데이터베이스를 검색하여 해당 카메라의 위치와 관련 기관 번호를 찾는다.
3. 1, 2번 과정에서 받은 이미지 데이터와 카메라 정보를 관련 기관에 전송한다.(해당 프로젝트에서는 라즈베리파이 LED에 점등하는 것으로 대체하였다.)

## 개발 내용
먼저, 쓰러진 사람과 동물 사체를 YOLO 알고리즘이 인식할 수 있도록 학습을 진행하였다. 쓰러진 사람은 일반적인 도로 상황에서 걸어다니거나 서있거나 뛰는 행동을 하는 사람을 제외한 누워있는 사람만을 인식한다. 동물 사체 역시 쓰러진 동물을 말한다. 카메라는 간단히 웹캠을 사용하였고, 서버 구성은 다음과 같다.
- YOLO 알고리즘이 동작하고 있는 카메라 서버: 쓰러진 사람과 동물 사체를 인식한다. 만약, 이를 인식하면 메인 서버에 인식된 이미지 데이터, 카메라 ID, 인식된 종류를 전송한다.
  
  *<프로토콜 형식>*
  ![form](https://user-images.githubusercontent.com/58102072/69716014-94366180-114c-11ea-8dc9-9129c2b15542.JPG)
  
  *<Camera <-> Main Server 통신>*
  ![client-mainserver](https://user-images.githubusercontent.com/58102072/69705290-98588400-1138-11ea-883c-f9182ba87a33.JPG)
- 모든 데이터 통신을 관리하는 메인 서버
  
  *<Main Server <-> Database 통신>*
  ![mainserver-database](https://user-images.githubusercontent.com/58102072/69716499-774e5e00-114d-11ea-9037-4eb0fb4b0abb.JPG)
  <br/>
  <br/>
  <br/>
  *<Main Server <-> 관련 기관 통신>*
  ![mainserver-Client](https://user-images.githubusercontent.com/58102072/69716715-f04db580-114d-11ea-8328-f56379547035.JPG)
  <br/>
  <br/>
- 데이터베이스: 카메라 ID, 카메라의 위치, 관련 기관 번호가 기본적으로 저장되어 있다.

  *<Database table 정보>*<br/>
  ![Database table](https://user-images.githubusercontent.com/58102072/69716817-2c811600-114e-11ea-8faa-e51ac7423020.jpg)
  <br/>
  <br/>
- 알람 서버(라즈베리 파이)

  <Alarm Client <-> Alarm Server>
  ![Alarm server](https://user-images.githubusercontent.com/58102072/69716961-7ff36400-114e-11ea-89a7-fcca0169a853.JPG)

YOLO 알고리즘과 서버 모두 C언어를 사용하여 구현하였다.
