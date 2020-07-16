#define TACT_START 2
#define TACT_DELAY 3
#define LED_READY 4
#define LED_DELAY 5

#define SEG_A3 6
#define SEG_A2 7
#define SEG_A1 8
#define SEG_A0 9

#define SEG_B3 10
#define SEG_B2 11
#define SEG_B1 12
#define SEG_B0 13

void setup() {
  pinMode(TACT_START, INPUT);
  pinMode(TACT_DELAY, INPUT);
  pinMode(LED_READY, OUTPUT);
  pinMode(LED_DELAY, OUTPUT);

  pinMode(SEG_A3, OUTPUT);
  pinMode(SEG_A2, OUTPUT);
  pinMode(SEG_A1, OUTPUT);
  pinMode(SEG_A0, OUTPUT);

  pinMode(SEG_B3, OUTPUT);
  pinMode(SEG_B2, OUTPUT);
  pinMode(SEG_B1, OUTPUT);
  pinMode(SEG_B0, OUTPUT);

  attachInterrupt(0, pressed_start, RISING);
  attachInterrupt(1, pressed_delay, RISING);
  
  Serial.begin(9600);
}

int packet_SYN_0 = 128;
int packet_SYN_1 = 0;

char packet_SYN_ACK[6] = { 0 };
int packet_ACK_0 = 64;
int packet_ACK_1 = 0;

char header;
int len;
char lvl;
char incoming[2] = { 0 };

int three_way_state = 0;
int delay_flag = 0;


int hits = 0;


char checkSYN;
char checkACK;
char checkFIN;
char checkLS;
char checkBELOW;
char checkRST;

int fin_state = 0; 
int state = 0;
char byte1 = 0;
char byte2 = 0;
char payload[10] = { 0 };

//level 1 cache values
int vb1 = 0;
int db1 = 0;
char tag1 = 0;
char data1[11] = {0};

//level2 cache values
int vbL2_0 = 0;
int vbL2_1 = 0;
int dbL2_0 = 0;
int dbL2_1 = 0;
char tagL2_0 = 0;
char tagL2_1 = 0;
char dataL2_0[11] = {0};
char dataL2_1[11] = {0};

//level 3 cache values
int vbL3_0 = 0;
int vbL3_1 = 0;
int vbL3_2 = 0;
int vbL3_3 = 0;
int vbL3_4 = 0;
int vbL3_5 = 0;
int vbL3_6 = 0;
int vbL3_7 = 0;
int dbL3_0 = 0;
int dbL3_1 = 0;
int dbL3_2 = 0;
int dbL3_3 = 0;
int dbL3_4 = 0;
int dbL3_5 = 0;
int dbL3_6 = 0;
int dbL3_7 = 0;
char tagL3_0 = 0;
char tagL3_1 = 0;
char tagL3_2 = 0;
char tagL3_3 = 0;
char tagL3_4 = 0;
char tagL3_5 = 0;
char tagL3_6 = 0;
char tagL3_7 = 0;
char dataL3_0[11] = {0};
char dataL3_1[11] = {0};
char dataL3_2[11] = {0};
char dataL3_3[11] = {0};
char dataL3_4[11] = {0};
char dataL3_5[11] = {0};
char dataL3_6[11] = {0};
char dataL3_7[11] = {0};

int LRU = 0;
int LRU_1 = 0;
int LRU_2 = 0;
int LRU_3 = 0;

int usedblock = 0;

int total = 0;
int check = 0;

void loop(){
  if(state == 1){
    int i = 0;    
    while(Serial.available() < 3);
    /* RECEIVE SYN ACK */
    while(i < 3){
      packet_SYN_ACK[i++] = Serial.read();
    }
    
    // initialize cache   
    lvl = packet_SYN_ACK[2];    
    init_cache(lvl);
    
    digitalWrite(LED_READY, HIGH);

    /* SEND ACK */
    Serial.write(packet_ACK_0);
    Serial.write(packet_ACK_1);
    state = 2;                                                                  
  }  
  
  if(state == 2){ // load/store protocol / start
   /* RECEIVE LOAD OR STORE */
   while(Serial.available() < 2);
   byte1 = Serial.read(); //header
   byte2 = Serial.read(); //length
   
   int len_int = byte2;
   while(Serial.available() < len_int); 
   for(int i = 0; i<len_int; i++) {
      payload[i] = Serial.read();
   }
   
    checkSYN = (byte1 & 0b10000000) >> 7;
    checkACK = (byte1 & 0b01000000) >> 6;
    checkFIN = (byte1 & 0b00100000) >> 5;
    checkLS = (byte1 & 0b00010000) >> 4;
    checkBELOW = (byte1 & 0b00001000) >> 3;
    checkRST = (byte1 & 0b00000100) >> 2;

    if(checkLS == 0) {
      state = 3;
    }else if(checkLS == 1) {
      state = 4; //store
        
    }else if(checkRST == 1) {
      state = 6;
    }
    if(checkFIN == 1) {
       fin_state = 1;
    }   
  }

  if(state == 3) {
    total++;
        if(lvl == 1){
          char tags = (payload[0] & 0b01111000) >> 3;  
          if (tags == tag1 && vb1 == 1) {
            /* SEND LOAD ACK */
            delayed();
            Serial.write(64);
            Serial.write(9);
            //Serial.write(payload[0]);
            //data1[2] = payload[0];   // set data[2] to MemAddr
            char word_addr = (payload[0] & 0b01111000) >> 3;
            data1[2] = word_addr; 
            for (int i = 2; i < 11; i++){
                Serial.write(data1[i]);
            }
            hits++;
          }else {
            if(vb1 == 0 && db1 == 0) {
                //NO DATA or Hindi ginagamit
            }else if(vb1 == 1 && db1 == 1) {
                /* SEND STORE BELOW */
                delayed();
                Serial.write(24); 
                Serial.write(9);
                Serial.write(payload[0]);
                for(int i = 0; i < 8; i++){
                  Serial.write(data1[i]);
                }
  
                /* RECEIVE STORE BELOW ACK */
                while(Serial.available() < 2);
                byte1 = Serial.read(); 
                byte2 = Serial.read();

                int j = 0;
                for (int i = 2; i < 10; i++){
                  data1[j] = payload[i];
                  j++;
                }
                db1 = 0;  
            }
  
            /* SEND LOAD BELOW */
            delayed();
            Serial.print((char)0b00001000);//Serial.write(8); // send header byte                              
            Serial.print((char)0b00000001); // send length byte             
            Serial.write(payload[0]); // send payload byte          
  
            int i = 0;
            
            /* RECEIVE LOAD BELOW ACK */
            while(Serial.available() < 11); // receive LOAD BELOW ACK packet
            while(i < 12){
               data1[i++] = Serial.read();//kuhaByte(); // kailangan header, length, payload bytes
            }
            // data1 = { head, len, payload...}
            // create and send LOAD ACK packet with data
            /*SEND LOAD ACK*/
            delayed();
            Serial.write(64);
            Serial.write(9);
            for(int i = 2; i < 11; i++){
              Serial.write(data1[i]);
            }
            tag1 = (payload[0] & 0b01111000) >> 3;
            vb1 = 1;
            db1 = 0;  
        }
                    
        
      }else if(lvl == 2) { 
          char tags2 = (payload[0] & 0b01110000) >> 4; 
          char index = (payload[0] & 0b00001000) >> 3;
          if (index == 0) {
            if (tags2 == tagL2_0 && vbL2_0 == 1) {
              /* SEND LOAD ACK */
              delayed();
              Serial.write(64);
              Serial.write(9);
              //Serial.write(payload[0]);
              //data1[2] = payload[0];   // set data[2] to MemAddr
              char word_addr = (payload[0] & 0b01111000) >> 3;
              dataL2_0[2] = word_addr; 
              for (int i = 2; i < 11; i++){
                  Serial.write(dataL2_0[i]);
              }
              hits++;
            }else {
              if(vbL2_0 == 0 && dbL2_0 == 0) {
                  //NO DATA or Hindi ginagamit
              }else if(vbL2_0 == 1 && dbL2_0 == 1) {
                  /* SEND STORE BELOW */
                  delayed();
                  Serial.write(24); 
                  Serial.write(9);
                  Serial.write(payload[0]);
                  for(int i = 0; i < 8; i++){
                    Serial.write(dataL2_0[i]);
                  }
    
                  /* RECEIVE STORE BELOW ACK */
                  while(Serial.available() < 2);
                  byte1 = Serial.read(); 
                  byte2 = Serial.read();
                  int j = 0;
                  for (int i = 2; i < 10; i++){
                    dataL2_0[j] = payload[i];
                    j++;
                  }
                  dbL2_0 = 0;  
              }
    
              /* SEND LOAD BELOW */
              delayed();
              Serial.print((char)0b00001000);//Serial.write(8); // send header byte                              
              Serial.print((char)0b00000001); // send length byte             
              Serial.write(payload[0]); // send payload byte          
    
              int i = 0;
              
              /* RECEIVE LOAD BELOW ACK */
              while(Serial.available() < 11); // receive LOAD BELOW ACK packet
              while(i < 12){
                 dataL2_0[i++] = Serial.read();//kuhaByte(); // kailangan header, length, payload bytes
              }
              // data1 = { head, len, payload...}
              // create and send LOAD ACK packet with data
              /*SEND LOAD ACK*/
              delayed();
              Serial.write(64);
              Serial.write(9);
              for(int i = 2; i < 11; i++){
                Serial.write(dataL2_0[i]);
              }
              tagL2_0 = (payload[0] & 0b01110000) >> 4;
              vbL2_0 = 1;   
              dbL2_0 = 0;  
            }// else (miss)  
          
            } //index 0
            else if(index == 1) { //index
              if (tags2 == tagL2_1 && vbL2_1 == 1) {
              /* SEND LOAD ACK */
              delayed();
              Serial.write(64);
              Serial.write(9);
              //Serial.write(payload[0]);
              //data1[2] = payload[0];   // set data[2] to MemAddr
              char word_addr = (payload[0] & 0b01111000) >> 3;
              dataL2_1[2] = word_addr; 
              for (int i = 2; i < 11; i++){
                  Serial.write(dataL2_1[i]);
              }
              hits++;
            }else {
              if(vbL2_1 == 0 && dbL2_1 == 0) {
                  //NO DATA or Hindi ginagamit
              }else if(vbL2_1 == 1 && dbL2_1 == 1) {
                  /* SEND STORE BELOW */
                  delayed();
                  Serial.write(24); 
                  Serial.write(9);
                  Serial.write(payload[0]);
                  for(int i = 0; i < 8; i++){
                    Serial.write(dataL2_1[i]);
                  }
    
                  /* RECEIVE STORE BELOW ACK */
                  while(Serial.available() < 2);
                  byte1 = Serial.read(); 
                  byte2 = Serial.read();
                  int j = 0;
                  for (int i = 2; i < 10; i++){
                    dataL2_1[j] = payload[i];
                    j++;
                  }
                  dbL2_1 = 0;  
              }
    
              /* SEND LOAD BELOW */
              Serial.print((char)0b00001000);//Serial.write(8); // send header byte                              
              Serial.print((char)0b00000001); // send length byte             
              Serial.write(payload[0]); // send payload byte          
    
              int i = 0;
              
              /* RECEIVE LOAD BELOW ACK */
              while(Serial.available() < 11); // receive LOAD BELOW ACK packet
              while(i < 12){
                 dataL2_1[i++] = Serial.read();//kuhaByte(); // kailangan header, length, payload bytes
              }
              // data1 = { head, len, payload...}
              // create and send LOAD ACK packet with data
              /*SEND LOAD ACK*/
              delayed();
              Serial.write(64);
              Serial.write(9);
              for(int i = 2; i < 11; i++){
                Serial.write(dataL2_1[i]);
              }
              tagL2_1 = (payload[0] & 0b01110000) >> 4;
              vbL2_1 = 1;   
              dbL2_1 = 0; 
            }// else (miss)  
            } //index 1
            
      }else if(lvl == 3){
        char tags3 = (payload[0] & 0b01100000) >> 5;
        char  index3 = (payload[0] & 0b00011000) >> 3;
        if (index3 == 0) {
          if (tags3 == tagL3_0 && vbL3_0 == 1) { ////////////hit on cache block 0

               /* SEND LOAD ACK */
              delayed();
              Serial.write(64);
              Serial.write(9);
              //Serial.write(payload[0]);
              //data1[2] = payload[0];   // set data[2] to MemAddr
              char word_addr = (payload[0] & 0b01111000) >> 3;
              dataL3_0[2] = word_addr; 
              for (int i = 2; i < 11; i++){
                  Serial.write(dataL3_0[i]);
              }
              hits++;
              LRU = 1; //LRU is the block na hindi ginamit (either this or yung isa, 2 choices lang)
              usedblock = 0; //cache block inside the index na we are currently using
          }
          else if (tags3 == tagL3_1 && vbL3_1 == 1) { ////////////hit on cache block 1
               /* SEND LOAD ACK */
              delayed();
              Serial.write(64);
              Serial.write(9);
              //Serial.write(payload[0]);
              //data1[2] = payload[0];   // set data[2] to MemAddr
              char word_addr = (payload[0] & 0b01111000) >> 3;
              dataL3_1[2] = word_addr; 
              for (int i = 2; i < 11; i++){
                  Serial.write(dataL3_1[i]);  
              }
              hits++;
              LRU = 0;
              usedblock = 1;
          }
          else { ////////////////////miss
              if(vbL3_0 == 0 && dbL3_0 == 0) { //free cache block 0
                  //LRU = 1;
                  usedblock = 0;
              }
              else if(vbL3_1 == 0 && dbL3_1 == 0) { //free chache block 1
                  //LRU = 0;
                  usedblock = 1;
              }
              else if((vbL3_0 == 1 && dbL3_0 == 1) || (vbL3_1 == 1 && dbL3_1 == 1)) { //if both vb is 1 use LRU
                 if (LRU == 0 && (vbL3_0 == 1 && dbL3_0 == 1)) { // send mo yung value ng cb#1, yun yung papalitan
                    /* SEND STORE BELOW */
                    delayed();
                    Serial.write(24); 
                    Serial.write(9);
                    Serial.write(payload[0]);
                    for(int i = 0; i < 8; i++){
                      Serial.write(dataL3_0[i]);
                    }
      
                    /* RECEIVE STORE BELOW ACK */
                    while(Serial.available() < 2);
                    byte1 = Serial.read(); 
                    byte2 = Serial.read();
                    int j = 0;
                    for (int i = 2; i < 10; i++){
                      dataL3_0[j] = payload[i];
                      j++;
                    }
                    dbL3_0 = 0; 
                    //LRU = 1;
                    usedblock = 0;
                 }
                 else if (LRU == 1 && (vbL3_1 == 1 && dbL3_1 == 1)) { // send mo yung value ng cb#0, yun yung papalitan
                    /* SEND STORE BELOW */
                    delayed();
                    Serial.write(24); 
                    Serial.write(9);
                    Serial.write(payload[0]);
                    for(int i = 0; i < 8; i++){
                      Serial.write(dataL3_1[i]);
                    }
      
                    /* RECEIVE STORE BELOW ACK */
                    while(Serial.available() < 2);
                    byte1 = Serial.read(); 
                    byte2 = Serial.read();
                    int j = 0;
                    for (int i = 2; i < 10; i++){
                      dataL3_1[j] = payload[i];
                      j++;
                    }
                    dbL3_1 = 0; 
                    //LRU = 0;
                    usedblock = 1;
                 }
              }
              /* SEND LOAD BELOW */
              delayed();
              Serial.print((char)0b00001000);//Serial.write(8); // send header byte                              
              Serial.print((char)0b00000001); // send length byte             
              Serial.write(payload[0]); // send payload byte          
              
              /* RECEIVE LOAD BELOW ACK */
              while(Serial.available() < 11); // receive LOAD BELOW ACK packet
              int i = 0;
              if (LRU == 0){
                while(i < 12){
                   dataL3_0[i++] = Serial.read();
                }
                // data1 = { head, len, payload...}
                // create and send LOAD ACK packet with data
                /*SEND LOAD ACK*/
                delayed();
                Serial.write(64);
                Serial.write(9);
                for(int i = 2; i < 11; i++){
                  Serial.write(dataL3_0[i]);
                }
                vbL3_0 = 1;
                dbL3_0 = 0; 
                tagL3_0 = tags3; 
                LRU = 1;
              }
              else if (LRU == 1) {
                while(i < 12){
                   dataL3_1[i++] = Serial.read();
                }
                // data1 = { head, len, payload...}
                // create and send LOAD ACK packet with data
                /*SEND LOAD ACK*/
                delayed();
                Serial.write(64);
                Serial.write(9);
                for(int i = 2; i < 11; i++){
                  Serial.write(dataL3_1[i]);
                }
                vbL3_1 = 1;
                dbL3_0 = 0; 
                tagL3_1 = tags3;  
                LRU = 0;         
            }//else if dirty
          } // else miss
        }else if(index3 == 1) {
          if (tags3 == tagL3_2 && vbL3_2 == 1) { ////////////hit on cache block 0

               /* SEND LOAD ACK */
              delayed();
              Serial.write(64);
              Serial.write(9);
              //Serial.write(payload[0]);
              //data1[2] = payload[0];   // set data[2] to MemAddr
              char word_addr = (payload[0] & 0b01111000) >> 3;
              dataL3_2[2] = word_addr; 
              for (int i = 2; i < 11; i++){
                  Serial.write(dataL3_2[i]);

              }
              hits++;
              //LRU = 0; //LRU is the block na hindi ginamit (either this or yung isa, 2 choices lang)
              usedblock = 0; //cache block inside the index na we are currently using
              LRU_1 = 1;
          }
          else if (tags3 == tagL3_3 && vbL3_3 == 1) { ////////////hit on cache block 1
               /* SEND LOAD ACK */
              delayed();
              Serial.write(64);
              Serial.write(9);
              //Serial.write(payload[0]);
              //data1[2] = payload[0];   // set data[2] to MemAddr
              char word_addr = (payload[0] & 0b01111000) >> 3;
              dataL3_3[2] = word_addr; 
              for (int i = 2; i < 11; i++){
                  Serial.write(dataL3_3[i]);  
              }
              hits++;
              //LRU = 1;
              usedblock = 1;
              LRU_1 = 0;
          }
          else { ////////////////////miss
              if(vbL3_2 == 0 && dbL3_2 == 0) { //free cache block 0
                  //LRU = 1;
                  usedblock = 0;
              }
              else if(vbL3_3 == 0 && dbL3_3 == 0) { //free chache block 1
                  //LRU = 0;
                  usedblock = 1;
              }
              else if((vbL3_2 == 1 && dbL3_2 == 1) || (vbL3_3 == 1 && dbL3_3 == 1)) { //if both vb is 1 use LRU
                 if (LRU_1 == 0 && (vbL3_2 == 1 && dbL3_2 == 1)) { // send mo yung value ng cb#1, yun yung papalitan
                    /* SEND STORE BELOW */
                    delayed();
                    Serial.write(24); 
                    Serial.write(9);
                    Serial.write(payload[0]);
                    for(int i = 0; i < 8; i++){
                      Serial.write(dataL3_2[i]);
                    }
      
                    /* RECEIVE STORE BELOW ACK */
                    while(Serial.available() < 2);
                    byte1 = Serial.read(); 
                    byte2 = Serial.read();
                    int j = 0;
                    for (int i = 2; i < 10; i++){
                      dataL3_2[j] = payload[i];
                      j++;
                    }
                    dbL3_2 = 0; 
                    //LRU = 1;
                    usedblock = 0;
                 }
                 else if (LRU_1 == 1 && (vbL3_3 == 1 && dbL3_3 == 1)) { // send mo yung value ng cb#0, yun yung papalitan
                    /* SEND STORE BELOW */
                    delayed();
                    Serial.write(24); 
                    Serial.write(9);
                    Serial.write(payload[0]);
                    for(int i = 0; i < 8; i++){
                      Serial.write(dataL3_3[i]);
                    }
      
                    /* RECEIVE STORE BELOW ACK */
                    while(Serial.available() < 2);
                    byte1 = Serial.read(); 
                    byte2 = Serial.read();
                    int j = 0;
                    for (int i = 2; i < 10; i++){
                      dataL3_3[j] = payload[i];
                      j++;
                    }
                    dbL3_3 = 0; 
                    //LRU_1 = 0;
                    usedblock = 1;
                 }
              }
              /* SEND LOAD BELOW */
              delayed();
              Serial.print((char)0b00001000);//Serial.write(8); // send header byte                              
              Serial.print((char)0b00000001); // send length byte             
              Serial.write(payload[0]); // send payload byte          
              
              /* RECEIVE LOAD BELOW ACK */
              while(Serial.available() < 11); // receive LOAD BELOW ACK packet
              int i = 0;
              if (LRU_1 == 0){
                while(i < 12){
                   dataL3_2[i++] = Serial.read();
                }
                // data1 = { head, len, payload...}
                // create and send LOAD ACK packet with data
                /*SEND LOAD ACK*/
                delayed();
                Serial.write(64);
                Serial.write(9);
                for(int i = 2; i < 11; i++){
                  Serial.write(dataL3_2[i]);
                }
                vbL3_2 = 1;
                dbL3_2 = 0; 
                tagL3_2 = tags3; 
                LRU_1 = 1;
              }
              else if (LRU_1 == 1) {
                while(i < 12){
                   dataL3_3[i++] = Serial.read();
                }
                // data1 = { head, len, payload...}
                // create and send LOAD ACK packet with data
                /*SEND LOAD ACK*/
                delayed();
                Serial.write(64);
                Serial.write(9);
                for(int i = 2; i < 11; i++){
                  Serial.write(dataL3_3[i]);
                }
                vbL3_3 = 1;
                dbL3_3 = 0; 
                tagL3_3 = tags3;  
                LRU_1 = 0;         
            }//else if dirty
          } // else miss  
        }else if(index3 == 2) {
          if (tags3 == tagL3_4 && vbL3_4 == 1) { ////////////hit on cache block 0

               /* SEND LOAD ACK */
              delayed();
              Serial.write(64);
              Serial.write(9);
              //Serial.write(payload[0]);
              //data1[2] = payload[0];   // set data[2] to MemAddr
              char word_addr = (payload[0] & 0b01111000) >> 3;
              dataL3_4[2] = word_addr; 
              for (int i = 2; i < 11; i++){
                  Serial.write(dataL3_4[i]);

              }
              LRU_2 = 1;
              hits++;
              //LRU = 0; //LRU is the block na hindi ginamit (either this or yung isa, 2 choices lang)
              usedblock = 0; //cache block inside the index na we are currently using
          }
          else if (tags3 == tagL3_5 && vbL3_5 == 1) { ////////////hit on cache block 1
               /* SEND LOAD ACK */
              delayed();
              Serial.write(64);
              Serial.write(9);
              //Serial.write(payload[0]);
              //data1[2] = payload[0];   // set data[2] to MemAddr
              char word_addr = (payload[0] & 0b01111000) >> 3;
              dataL3_5[2] = word_addr; 
              for (int i = 2; i < 11; i++){
                  Serial.write(dataL3_5[i]);  
              }
              hits++;
              LRU_2 = 0;
              usedblock = 1;
          }
          else { ////////////////////miss
              if(vbL3_4 == 0 && dbL3_4 == 0) { //free cache block 0
                  //LRU = 1;
                  usedblock = 0;
              }
              else if(vbL3_5 == 0 && dbL3_5 == 0) { //free chache block 1
                  //LRU = 0;
                  usedblock = 1;
              }
              else if((vbL3_4 == 1 && dbL3_4 == 1) || (vbL3_5 == 1 && dbL3_5 == 1)) { //if both vb is 1 use LRU
                 if (LRU_2 == 0 && (vbL3_4 == 1 && dbL3_4 == 1)) { // send mo yung value ng cb#1, yun yung papalitan
                    /* SEND STORE BELOW */
                    delayed();
                    Serial.write(24); 
                    Serial.write(9);
                    Serial.write(payload[0]);
                    for(int i = 0; i < 8; i++){
                      Serial.write(dataL3_4[i]);
                    }
      
                    /* RECEIVE STORE BELOW ACK */
                    while(Serial.available() < 2);
                    byte1 = Serial.read(); 
                    byte2 = Serial.read();
                    int j = 0;
                    for (int i = 2; i < 10; i++){
                      dataL3_4[j] = payload[i];
                      j++;
                    }
                    dbL3_4 = 0; 
                    //LRU = 1;
                    usedblock = 0;
                 }
                 else if (LRU_2 == 1 && (vbL3_5 == 1 && dbL3_5 == 1)) { // send mo yung value ng cb#0, yun yung papalitan
                    /* SEND STORE BELOW */
                    delayed();
                    Serial.write(24); 
                    Serial.write(9);
                    Serial.write(payload[0]);
                    for(int i = 0; i < 8; i++){
                      Serial.write(dataL3_5[i]);
                    }
      
                    /* RECEIVE STORE BELOW ACK */
                    while(Serial.available() < 2);
                    byte1 = Serial.read(); 
                    byte2 = Serial.read();
                    int j = 0;
                    for (int i = 2; i < 10; i++){
                      dataL3_5[j] = payload[i];
                      j++;
                    }
                    dbL3_5 = 0; 
                    //LRU = 0;
                    usedblock = 1;
                 }
              }
              /* SEND LOAD BELOW */
              delayed();
              Serial.print((char)0b00001000);//Serial.write(8); // send header byte                              
              Serial.print((char)0b00000001); // send length byte             
              Serial.write(payload[0]); // send payload byte          
              
              /* RECEIVE LOAD BELOW ACK */
              while(Serial.available() < 11); // receive LOAD BELOW ACK packet
              int i = 0;
              if (LRU_2 == 0){
                while(i < 12){
                   dataL3_4[i++] = Serial.read();
                }
                // data1 = { head, len, payload...}
                // create and send LOAD ACK packet with data
                /*SEND LOAD ACK*/
                delayed();
                Serial.write(64);
                Serial.write(9);
                for(int i = 2; i < 11; i++){
                  Serial.write(dataL3_4[i]);
                }
                vbL3_4 = 1;
                dbL3_4 = 0; 
                tagL3_4 = tags3; 
                LRU_2 = 1;
              }
              else if (LRU_2 == 1) {
                while(i < 12){
                   dataL3_5[i++] = Serial.read();
                }
                // data1 = { head, len, payload...}
                // create and send LOAD ACK packet with data
                /*SEND LOAD ACK*/
                delayed();
                Serial.write(64);
                Serial.write(9);
                for(int i = 2; i < 11; i++){
                  Serial.write(dataL3_5[i]);
                }
                vbL3_5 = 1;
                dbL3_4 = 0; 
                tagL3_5 = tags3;  
                LRU_2 = 0;         
            }//else if dirty
          } // else miss
        }else if(index3 == 3) {
             if (tags3 == tagL3_6 && vbL3_6 == 1) { ////////////hit on cache block 0

               /* SEND LOAD ACK */
              delayed();
              Serial.write(64);
              Serial.write(9);
              //Serial.write(payload[0]);
              //data1[2] = payload[0];   // set data[2] to MemAddr
              char word_addr = (payload[0] & 0b01111000) >> 3;
              dataL3_6[2] = word_addr; 
              for (int i = 2; i < 11; i++){
                  Serial.write(dataL3_6[i]);

              }
              hits++;
              //LRU = 0; //LRU is the block na hindi ginamit (either this or yung isa, 2 choices lang)
              usedblock = 0; //cache block inside the index na we are currently using
              LRU_3 = 1;
          }
          else if (tags3 == tagL3_7 && vbL3_7 == 1) { ////////////hit on cache block 1
               /* SEND LOAD ACK */
              delayed();
              Serial.write(64);
              Serial.write(9);
              //Serial.write(payload[0]);
              //data1[2] = payload[0];   // set data[2] to MemAddr
              char word_addr = (payload[0] & 0b01111000) >> 3;
              dataL3_7[2] = word_addr; 
              for (int i = 2; i < 11; i++){
                  Serial.write(dataL3_7[i]);  
              }
              hits++;
              LRU_3 = 0;
              usedblock = 1;
          }
          else { ////////////////////miss
              if(vbL3_6 == 0 && dbL3_6 == 0) { //free cache block 0
                  //LRU = 1;
                  usedblock = 0;
              }
              else if(vbL3_7 == 0 && dbL3_7 == 0) { //free chache block 1
                  //LRU = 0;
                  usedblock = 1;
              }
              else if((vbL3_6 == 1 && dbL3_6 == 1) || (vbL3_7 == 1 && dbL3_7 == 1)) { //if both vb is 1 use LRU
                 if (LRU_3 == 0) { // send mo yung value ng cb#1, yun yung papalitan
                    /* SEND STORE BELOW */
                    delayed();
                    Serial.write(24); 
                    Serial.write(9);
                    Serial.write(payload[0]);
                    for(int i = 0; i < 8; i++){
                      Serial.write(dataL3_6[i]);
                    }
      
                    /* RECEIVE STORE BELOW ACK */
                    while(Serial.available() < 2);
                    byte1 = Serial.read(); 
                    byte2 = Serial.read();
                    int j = 0;
                    for (int i = 2; i < 10; i++){
                      dataL3_6[j] = payload[i];
                      j++;
                    }
                    dbL3_6 = 0; 
                    //LRU = 1;
                    usedblock = 0;
                 }
                 else if (LRU_3 == 1) { // send mo yung value ng cb#0, yun yung papalitan
                    /* SEND STORE BELOW */
                    delayed();
                    Serial.write(24); 
                    Serial.write(9);
                    Serial.write(payload[0]);
                    for(int i = 0; i < 8; i++){
                      Serial.write(dataL3_7[i]);
                    }
      
                    /* RECEIVE STORE BELOW ACK */
                    while(Serial.available() < 2);
                    byte1 = Serial.read(); 
                    byte2 = Serial.read();
                    int j = 0;
                    for (int i = 2; i < 10; i++){
                      dataL3_7[j] = payload[i];
                      j++;
                    }
                    dbL3_7 = 0; 
                    //LRU = 0;
                    usedblock = 1;
                 }
              }
              /* SEND LOAD BELOW */
              delayed();
              Serial.print((char)0b00001000);//Serial.write(8); // send header byte                              
              Serial.print((char)0b00000001); // send length byte             
              Serial.write(payload[0]); // send payload byte          
              
              /* RECEIVE LOAD BELOW ACK */
              while(Serial.available() < 11); // receive LOAD BELOW ACK packet
              int i = 0;
              if (LRU_3 == 0){
                while(i < 12){
                   dataL3_6[i++] = Serial.read();
                }
                // data1 = { head, len, payload...}
                // create and send LOAD ACK packet with data
                /*SEND LOAD ACK*/
                delayed();
                Serial.write(64);
                Serial.write(9);
                for(int i = 2; i < 11; i++){
                  Serial.write(dataL3_6[i]);
                }
                vbL3_6 = 1;
                dbL3_6 = 0; 
                tagL3_6 = tags3; 
                LRU_3 = 1;
              }
              else if (LRU_3 == 1) {
                while(i < 12){
                   dataL3_7[i++] = Serial.read();
                }
                // data1 = { head, len, payload...}
                // create and send LOAD ACK packet with data
                /*SEND LOAD ACK*/
                delayed();
                Serial.write(64);
                Serial.write(9);
                for(int i = 2; i < 11; i++){
                  Serial.write(dataL3_7[i]);
                }
                vbL3_7 = 1;
                dbL3_7 = 0; 
                tagL3_7 = tags3;  
                LRU_3 = 0;         
            }//else if dirty
          } // else miss
        }
      }
        
      //State change
      state = 2;
      check = 1;
  }

  if(state == 4) {
    total++;
    if (lvl == 1) {
       char tags = (payload[0] & 0b01111000) >> 3;
       if(tags == tag1 && vb1 == 1) {
          db1 = 1;
          hits++;   
       }
       else {
        if (vb1 == 1 && db1 == 1) { //if miss  
          /* STORE BELOW */
          delayed();
          Serial.write(24); //store below
          Serial.write(9);
          char word_addr = (payload[0] & 0b01111000) >> 3;
          data1[2] = word_addr;
          for(int i = 2; i < 11; i++){
            Serial.write(data1[i]);
          }
          /* RECEIVE STORE BELOW ACK */
          while(Serial.available() < 2);
          byte1 = Serial.read(); //store below ack
          byte2 = Serial.read();
          int j = 0;
          for (int i = 2; i < 10; i++){
            data1[j] = payload[i];
            j++;
          }
        }
          /* SEND LOAD BELOW */
          delayed();
          Serial.print((char)0b00001000);//Serial.write(8); // send header byte                              
          Serial.print((char)0b00000001); // send length byte             
          Serial.write(payload[0]); // send payload byte          

          int i = 0;
          /* RECEIVE LOAD BELOW ACK */
          while(Serial.available() < 11); // receive LOAD BELOW ACK packet
          while(i < 12){
             data1[i++] = Serial.read(); // kailangan header, length, payload bytes
          }
          // data1 = { head, len, payload...}
          // create and send LOAD ACK packet with data
          /*SEND LOAD ACK */
          delayed();
          Serial.write(64);
          Serial.write(9);
          for(int i = 2; i < 11; i++){
            Serial.write(data1[i]);
          }
          db1 = 1;
        }
          tag1 = (payload[0] & 0b01111000) >> 3;
          vb1 = 1;  
          int j=0;
          for(int i = 3; i<12; i++) {
            data1[j] = payload[i];
            j++; 
          }
       /* SEND STORE ACK */
       delayed();
       db1 = 1;
       Serial.write(80); //header of STORE ACK
       Serial.write(0); //length of STORE ACK
    
    } else if (lvl == 2) {
      char tags2 = (payload[0] & 0b01110000) >> 4;
      char index = (payload[0] & 0b00001000) >> 3;

      if(index == 0) {
         if(tags2 == tagL2_0 && vbL2_0 == 1) {
            hits++;
            dbL2_0 = 1;   
         }else { 
          if (vbL2_0 == 1 && dbL2_0 == 1) { //if miss  
            /* STORE BELOW */
            delayed();
            Serial.write(24); //store below
            Serial.write(9);
            char word_addr = (payload[0] & 0b01111000) >> 3;
            dataL2_0[2] = word_addr;
            for(int i = 2; i < 11; i++){
              Serial.write(dataL2_0[i]);
            }
            
            /* RECEIVE STORE BELOW ACK */
            while(Serial.available() < 2);
            byte1 = Serial.read(); //store below ack
            byte2 = Serial.read();
            int j = 0;
            for (int i = 2; i < 10; i++){
              dataL2_0[j] = payload[i];
              j++;
            }
            dbL2_0 = 1;
         }
            /* SEND LOAD BELOW */
            delayed();
            Serial.print((char)0b00001000);//Serial.write(8); // send header byte                              
            Serial.print((char)0b00000001); // send length byte             
            Serial.write(payload[0]); // send payload byte          

            int i = 0;
            /* RECEIVE LOAD BELOW ACK */
            while(Serial.available() < 11); // receive LOAD BELOW ACK packet
            while(i < 12){
               dataL2_0[i++] = Serial.read(); // kailangan header, length, payload bytes
            }
            // data1 = { head, len, payload...}
            // create and send LOAD ACK packet with data
            /*SEND LOAD ACK */
            delayed();
            Serial.write(64);
            Serial.write(9);
            for(int i = 2; i < 11; i++){
              Serial.write(dataL2_0[i]);
            }
        }
         tagL2_0 = (payload[0] & 0b01110000) >> 4;
         vbL2_0 = 1; 
         dbL2_0 = 1;
         
         // update data of chosen cache block
         int j=0;
         for(int i = 3; i<12; i++) {
            dataL2_0[j] = payload[i];
            j++; 
         }
         /* SEND STORE ACK */
         delayed();
         Serial.write(80); //header of STORE ACK
         Serial.write(0); //length of STORE ACK
      } //index 0
      else if (index == 1) {
        if(tags2 == tagL2_1 && vbL2_1 == 1) {
            hits++;   
            dbL2_1 = 1;
         } else {
          if (vbL2_1 == 1 && dbL2_1 == 1) { //if miss  
            /* STORE BELOW */
            delayed();
            Serial.write(24); //store below
            Serial.write(9);
            char word_addr = (payload[0] & 0b01111000) >> 3;
            dataL2_1[2] = word_addr;
            for(int i = 2; i < 11; i++){
              Serial.write(dataL2_1[i]);
            }
            
            /* RECEIVE STORE BELOW ACK */
            while(Serial.available() < 2);
            byte1 = Serial.read(); //store below ack
            byte2 = Serial.read();
            int j = 0;
            for (int i = 2; i < 10; i++){
              dataL2_1[j] = payload[i];
              j++;
            }
          }
            /* SEND LOAD BELOW */
            Serial.print((char)0b00001000);//Serial.write(8); // send header byte                              
            Serial.print((char)0b00000001); // send length byte             
            Serial.write(payload[0]); // send payload byte          

            int i = 0;
            /* RECEIVE LOAD BELOW ACK */
            while(Serial.available() < 11); // receive LOAD BELOW ACK packet
            while(i < 12){
               dataL2_1[i++] = Serial.read(); // kailangan header, length, payload bytes
            }
            // data1 = { head, len, payload...}
            // create and send LOAD ACK packet with data
            /*SEND LOAD ACK */
            delayed();
            Serial.write(64);
            Serial.write(9);
            for(int i = 2; i < 11; i++){
              Serial.write(dataL2_1[i]);
            }
            dbL2_1 = 1;
        }
         tagL2_1 = (payload[0] & 0b01110000) >> 4;
         vbL2_1 = 1;  
         dbL2_1 = 1;         
         // update data of chosen cache block
         int j=0;
         for(int i = 3; i<12; i++) {
            dataL2_1[j] = payload[i];
            j++; 
         }
         /* SEND STORE ACK */
         delayed();
         Serial.write(80); //header of STORE ACK
         Serial.write(0); //length of STORE ACK
      } //index 1
      
    } else if (lvl == 3) {
      char tags3 = (payload[0] & 0b01100000) >> 5;
      char index3 = (payload[0] & 0b00011000) >> 3;
      if (index3 == 0) {
        if (tags3 == tagL3_0 && vbL3_0 == 1) {
          hits++;
          LRU = 1;
          dbL3_0 = 1;
        }
        else if (tags3 == tagL3_1 && vbL3_1 == 1) {
          hits++;
          LRU = 0;
          dbL3_1 = 1;
        }
        else { //miss
          if(vbL3_0 == 0 && dbL3_0 == 0) { //free cache block 0
            dbL3_0 = 1;
          }
          else if(vbL3_1 == 0 && dbL3_1 == 0) { //free chache block 1
            dbL3_1 = 1;
          }
          if (vbL3_0 == 1 && dbL3_0 == 1 || vbL3_1 == 1 && dbL3_1 == 1) {
            if (LRU == 0 && (vbL3_0 == 1 && dbL3_0 == 1)) { // send mo yung value ng cb#1, yun yung papalitan
                /* SEND STORE BELOW */
                delayed();
                Serial.write(24); 
                Serial.write(9);
                Serial.write(payload[0]);
                for(int i = 0; i < 8; i++){
                  Serial.write(dataL3_0[i]);
                }
  
                /* RECEIVE STORE BELOW ACK */
                while(Serial.available() < 2);
                byte1 = Serial.read(); 
                byte2 = Serial.read();
                int j = 0;
                for (int i = 2; i < 10; i++){
                  dataL3_0[j] = payload[i];
                  j++;
                }
                dbL3_0 = 1; 
                //LRU = 1;
                usedblock = 0;
            }
              else if (LRU == 1 && (vbL3_1 == 1 && dbL3_1 == 1)) { // send mo yung value ng cb#0, yun yung papalitan
                  /* SEND STORE BELOW */
                  delayed();
                  Serial.write(24); 
                  Serial.write(9);
                  Serial.write(payload[0]);
                  for(int i = 0; i < 8; i++){
                    Serial.write(dataL3_1[i]);
                  }
    
                  /* RECEIVE STORE BELOW ACK */
                  while(Serial.available() < 2);
                  byte1 = Serial.read(); 
                  byte2 = Serial.read();
                  int j = 0;
                  for (int i = 2; i < 10; i++){
                    dataL3_1[j] = payload[i];
                    j++;
                  }
                  dbL3_1 = 1; 
                  //LRU = 0;
                  usedblock = 1;
              }  
          } //if dirty
              /* SEND LOAD BELOW */
          delayed();
          Serial.print((char)0b00001000);//Serial.write(8); // send header byte                              
          Serial.print((char)0b00000001); // send length byte             
          Serial.write(payload[0]); // send payload byte          
          
        /* RECEIVE LOAD BELOW ACK */
        while(Serial.available() < 11); // receive LOAD BELOW ACK packet
        int i = 0;
        if (LRU == 0){
            while(i < 12){
               dataL3_0[i++] = Serial.read();
            }
            // data1 = { head, len, payload...}
            // create and send LOAD ACK packet with data
            /*SEND LOAD ACK*/
            delayed();
            Serial.write(64);
            Serial.write(9);
            for(int i = 2; i < 11; i++){
              Serial.write(dataL3_0[i]);
            }
            vbL3_0 = 1;
            tagL3_0 = tags3; 
            dbL3_0 = 1;
            LRU = 1;
        }
        else if (LRU == 1) {
            while(i < 12){
               dataL3_1[i++] = Serial.read();
            }
            // data1 = { head, len, payload...}
            // create and send LOAD ACK packet with data
            /*SEND LOAD ACK*/
            delayed();
            Serial.write(64);
            Serial.write(9);
            for(int i = 2; i < 11; i++){
              Serial.write(dataL3_1[i]);
            }
            vbL3_1 = 1;
            tagL3_1 = tags3;
            dbL3_1 = 1;  
            LRU = 0;         
        }
        }//else miss
        // update data of chosen cache block
        if(LRU == 0) {
              int j=0;
              for(int i = 3; i<12; i++) {
                  dataL3_0[j] = payload[i];
                  j++; 
              }
              dbL3_0 = 1;  
          }
          else if(LRU == 1) {
              int j=0;
              for(int i = 3; i<12; i++) {
                  dataL3_1[j] = payload[i];
                  j++; 
              }
              dbL3_1 = 1;  
          }
             /* SEND STORE ACK */
             delayed();
             Serial.write(80); //header of STORE ACK
             Serial.write(0); //length of STORE ACK
             //state = 2;

             
      }else if (index3 == 1) {
        if (tags3 == tagL3_2 && vbL3_2 == 1) {
          hits++;
          LRU_1 = 1;
          dbL3_2 = 1;
        }
        else if (tags3 == tagL3_3 && vbL3_3 == 1) {
          hits++;
          LRU_1 = 0;
          dbL3_3 = 1;
        }
        else { //miss
          if(vbL3_2 == 0 && dbL3_2 == 0) { //free cache block 0
            dbL3_2 = 1;
          }
          else if(vbL3_3 == 0 && dbL3_3 == 0) { //free chache block 1
            dbL3_3 = 1;
          }
          if (vbL3_2 == 1 && dbL3_2 == 1 || vbL3_3 == 1 && dbL3_3 == 1) {
            if (LRU_1 == 0 && (vbL3_2 == 1 && dbL3_2 == 1)) { // send mo yung value ng cb#1, yun yung papalitan
                /* SEND STORE BELOW */
                delayed();
                Serial.write(24); 
                Serial.write(9);
                Serial.write(payload[0]);
                for(int i = 0; i < 8; i++){
                  Serial.write(dataL3_2[i]);
                }
  
                /* RECEIVE STORE BELOW ACK */
                while(Serial.available() < 2);
                byte1 = Serial.read(); 
                byte2 = Serial.read();
                int j = 0;
                for (int i = 2; i < 10; i++){
                  dataL3_2[j] = payload[i];
                  j++;
                }
                dbL3_2 = 1; 
                //LRU = 1;
                usedblock = 0;
            }
              else if (LRU_1 == 1 && (vbL3_3 == 1 && dbL3_3 == 1)) { // send mo yung value ng cb#0, yun yung papalitan
                  /* SEND STORE BELOW */
                  delayed();
                  Serial.write(24); 
                  Serial.write(9);
                  Serial.write(payload[0]);
                  for(int i = 0; i < 8; i++){
                    Serial.write(dataL3_3[i]);
                  }
    
                  /* RECEIVE STORE BELOW ACK */
                  while(Serial.available() < 2);
                  byte1 = Serial.read(); 
                  byte2 = Serial.read();
                  int j = 0;
                  for (int i = 2; i < 10; i++){
                    dataL3_3[j] = payload[i];
                    j++;
                  }
                  dbL3_3 = 1; 
                  //LRU = 0;
                  usedblock = 1;
              }  
          } //if dirty
              /* SEND LOAD BELOW */
          delayed();
          Serial.print((char)0b00001000);//Serial.write(8); // send header byte                              
          Serial.print((char)0b00000001); // send length byte             
          Serial.write(payload[0]); // send payload byte          
          
        /* RECEIVE LOAD BELOW ACK */
        while(Serial.available() < 11); // receive LOAD BELOW ACK packet
        int i = 0;
        if (LRU_1 == 0){
            while(i < 12){
               dataL3_2[i++] = Serial.read();
            }
            // data1 = { head, len, payload...}
            // create and send LOAD ACK packet with data
            /*SEND LOAD ACK*/
            delayed();
            Serial.write(64);
            Serial.write(9);
            for(int i = 2; i < 11; i++){
              Serial.write(dataL3_2[i]);
            }
            vbL3_2 = 1;
            tagL3_2 = tags3; 
            dbL3_2 = 1;
            LRU_1 = 1;
        }
        else if (LRU_1 == 1) {
            while(i < 12){
               dataL3_3[i++] = Serial.read();
            }
            // data1 = { head, len, payload...}
            // create and send LOAD ACK packet with data
            /*SEND LOAD ACK*/
            delayed();
            Serial.write(64);
            Serial.write(9);
            for(int i = 2; i < 11; i++){
              Serial.write(dataL3_3[i]);
            }
            vbL3_3 = 1;
            tagL3_3 = tags3;
            dbL3_3 = 1;  
            LRU_1 = 0;         
        }
        }//else miss
        // update data of chosen cache block
        if(LRU_1 == 0) {
              int j=0;
              for(int i = 3; i<12; i++) {
                  dataL3_2[j] = payload[i];
                  j++; 
              }
              dbL3_2 = 1;  
          }
          else if(LRU_1 == 1) {
              int j=0;
              for(int i = 3; i<12; i++) {
                  dataL3_3[j] = payload[i];
                  j++; 
              }
              dbL3_3 = 1;  
          }
             /* SEND STORE ACK */
             delayed();
             Serial.write(80); //header of STORE ACK
             Serial.write(0); //length of STORE ACK
             //state = 2;
      }
      
      else if (index3 == 2) {
        if (tags3 == tagL3_4 && vbL3_4 == 1) {
          hits++;
          LRU_2 = 1;
          dbL3_4 = 1;
        }
        else if (tags3 == tagL3_5 && vbL3_5 == 1) {
          hits++;
          LRU_2 = 0;
          dbL3_5 = 1;
        }
        else { //miss
          if(vbL3_4 == 0 && dbL3_4 == 0) { //free cache block 0
            dbL3_4 = 1;
          }
          else if(vbL3_5 == 0 && dbL3_5 == 0) { //free chache block 1
            dbL3_5 = 1;
          }
          if (vbL3_4 == 1 && dbL3_4 == 1 || vbL3_5 == 1 && dbL3_5 == 1) {
            if (LRU_2 == 0 && (vbL3_4 == 1 && dbL3_4 == 1)) { // send mo yung value ng cb#1, yun yung papalitan
                /* SEND STORE BELOW */
                delayed();
                Serial.write(24); 
                Serial.write(9);
                Serial.write(payload[0]);
                for(int i = 0; i < 8; i++){
                  Serial.write(dataL3_4[i]);
                }
  
                /* RECEIVE STORE BELOW ACK */
                while(Serial.available() < 2);
                byte1 = Serial.read(); 
                byte2 = Serial.read();
                int j = 0;
                for (int i = 2; i < 10; i++){
                  dataL3_4[j] = payload[i];
                  j++;
                }
                dbL3_4 = 1; 
                //LRU = 1;
                usedblock = 0;
            }
              else if (LRU_2 == 1 && (vbL3_5 == 1 && dbL3_5 == 1)) { // send mo yung value ng cb#0, yun yung papalitan
                  /* SEND STORE BELOW */
                  delayed();
                  Serial.write(24); 
                  Serial.write(9);
                  Serial.write(payload[0]);
                  for(int i = 0; i < 8; i++){
                    Serial.write(dataL3_5[i]);
                  }
    
                  /* RECEIVE STORE BELOW ACK */
                  while(Serial.available() < 2);
                  byte1 = Serial.read(); 
                  byte2 = Serial.read();
                  int j = 0;
                  for (int i = 2; i < 10; i++){
                    dataL3_5[j] = payload[i];
                    j++;
                  }
                  dbL3_5 = 1; 
                  //LRU = 0;
                  usedblock = 1;
              }  
          } //if dirty
              /* SEND LOAD BELOW */
          delayed();
          Serial.print((char)0b00001000);//Serial.write(8); // send header byte                              
          Serial.print((char)0b00000001); // send length byte             
          Serial.write(payload[0]); // send payload byte          
          
        /* RECEIVE LOAD BELOW ACK */
        while(Serial.available() < 11); // receive LOAD BELOW ACK packet
        int i = 0;
        if (LRU_2 == 0){
            while(i < 12){
               dataL3_4[i++] = Serial.read();
            }
            // data1 = { head, len, payload...}
            // create and send LOAD ACK packet with data
            /*SEND LOAD ACK*/
            delayed();
            Serial.write(64);
            Serial.write(9);
            for(int i = 2; i < 11; i++){
              Serial.write(dataL3_4[i]);
            }
            vbL3_4 = 1;
            tagL3_4 = tags3; 
            dbL3_4 = 1;
            LRU_2 = 1;
        }
        else if (LRU_2 == 1) {
            while(i < 12){
               dataL3_5[i++] = Serial.read();
            }
            // data1 = { head, len, payload...}
            // create and send LOAD ACK packet with data
            /*SEND LOAD ACK*/
            delayed();
            Serial.write(64);
            Serial.write(9);
            for(int i = 2; i < 11; i++){
              Serial.write(dataL3_5[i]);
            }
            vbL3_5 = 1;
            tagL3_5 = tags3;
            dbL3_5 = 1;  
            LRU_2 = 0;         
        }
        }//else miss
        // update data of chosen cache block
        if(LRU_2 == 0) {
              int j=0;
              for(int i = 3; i<12; i++) {
                  dataL3_4[j] = payload[i];
                  j++; 
              }
              dbL3_4 = 1;  
          }
          else if(LRU_2 == 1) {
              int j=0;
              for(int i = 3; i<12; i++) {
                  dataL3_5[j] = payload[i];
                  j++; 
              }
              dbL3_5 = 1;  
          }
             /* SEND STORE ACK */
             delayed();
             Serial.write(80); //header of STORE ACK
             Serial.write(0); //length of STORE ACK
             //state = 2;
      
      
      }else if (index3 == 3) {
        if (tags3 == tagL3_6 && vbL3_6 == 1) {
          hits++;
          LRU_3 = 1;
          dbL3_6 = 1;
        }
        else if (tags3 == tagL3_7 && vbL3_7 == 1) {
          hits++;
          LRU_3 = 0;
          dbL3_7 = 1;
        }
        else { //miss
          if(vbL3_6 == 0 && dbL3_6 == 0) { //free cache block 0
            dbL3_6 = 1;
          }
          else if(vbL3_7 == 0 && dbL3_7 == 0) { //free chache block 1
            dbL3_7 = 1;
          }
          if (vbL3_6 == 1 && dbL3_6 == 1 || vbL3_7 == 1 && dbL3_7 == 1) {
            if (LRU_3 == 0 && (vbL3_6 == 1 && dbL3_6 == 1)) { // send mo yung value ng cb#1, yun yung papalitan
                /* SEND STORE BELOW */
                delayed();
                Serial.write(24); 
                Serial.write(9);
                Serial.write(payload[0]);
                for(int i = 0; i < 8; i++){
                  Serial.write(dataL3_6[i]);
                }
  
                /* RECEIVE STORE BELOW ACK */
                while(Serial.available() < 2);
                byte1 = Serial.read(); 
                byte2 = Serial.read();
                int j = 0;
                for (int i = 2; i < 10; i++){
                  dataL3_6[j] = payload[i];
                  j++;
                }
                dbL3_6 = 1; 
                //LRU = 1;
                usedblock = 0;
            }
              else if (LRU_3 == 1 && (vbL3_7 == 1 && dbL3_7 == 1)) { // send mo yung value ng cb#0, yun yung papalitan
                  /* SEND STORE BELOW */
                  delayed();
                  Serial.write(24); 
                  Serial.write(9);
                  Serial.write(payload[0]);
                  for(int i = 0; i < 8; i++){
                    Serial.write(dataL3_7[i]);
                  }
    
                  /* RECEIVE STORE BELOW ACK */
                  while(Serial.available() < 2);
                  byte1 = Serial.read(); 
                  byte2 = Serial.read();
                  int j = 0;
                  for (int i = 2; i < 10; i++){
                    dataL3_7[j] = payload[i];
                    j++;
                  }
                  dbL3_7 = 1; 
                  //LRU = 0;
                  usedblock = 1;
              }  
          } //if dirty
              /* SEND LOAD BELOW */
          delayed();
          Serial.print((char)0b00001000);//Serial.write(8); // send header byte                              
          Serial.print((char)0b00000001); // send length byte             
          Serial.write(payload[0]); // send payload byte          
          
        /* RECEIVE LOAD BELOW ACK */
        while(Serial.available() < 11); // receive LOAD BELOW ACK packet
        int i = 0;
        if (LRU_3 == 0){
            while(i < 12){
               dataL3_6[i++] = Serial.read();
            }
            // data1 = { head, len, payload...}
            // create and send LOAD ACK packet with data
            /*SEND LOAD ACK*/
            delayed();
            Serial.write(64);
            Serial.write(9);
            for(int i = 2; i < 11; i++){
              Serial.write(dataL3_6[i]);
            }
            vbL3_6 = 1;
            tagL3_6 = tags3; 
            dbL3_6 = 1;
            LRU_3 = 1;
        }
        else if (LRU_3 == 1) {
            while(i < 12){
               dataL3_7[i++] = Serial.read();
            }
            // data1 = { head, len, payload...}
            // create and send LOAD ACK packet with data
            /*SEND LOAD ACK*/
            delayed();
            Serial.write(64);
            Serial.write(9);
            for(int i = 2; i < 11; i++){
              Serial.write(dataL3_7[i]);
            }
            vbL3_7 = 1;
            tagL3_7 = tags3;
            dbL3_7 = 1;  
            LRU_3 = 0;         
        }
        }//else miss
        // update data of chosen cache block
        if(LRU_3 == 0) {
              int j=0;
              for(int i = 3; i<12; i++) {
                  dataL3_6[j] = payload[i];
                  j++; 
              }
              dbL3_6 = 1;  
          }
          else if(LRU_3 == 1) {
              int j=0;
              for(int i = 3; i<12; i++) {
                  dataL3_7[j] = payload[i];
                  j++; 
              }
              dbL3_7 = 1;  
          }
             /* SEND STORE ACK */
             delayed();
             Serial.write(80); //header of STORE ACK
             Serial.write(0); //length of STORE ACK
             //state = 2;  
      }
    }
    state = 2;
  }
  //RESET
  if (state == 6){
    init_cache(lvl);
  }

  if (fin_state == 1){
    int hit1 = (hits & 0x0ff0);
    int hit2 = (hits & 0x000f) << 4;
    int access1 = (total & 0x0f00) >> 8;
    int access2 = (total & 0x00ff);
    char byte1 = hit1 >> 4;
    char byte2 = hit2 + access1;
    char byte3 = access2;

    /* SEND FIN ACK */
    delayed();
    Serial.write(96);
    Serial.write(3);
    Serial.write(byte1);
    Serial.write(byte2);
    Serial.write(byte3);
  }
  
  show_total(total);
}

void init_cache(char level){
  hits = 0;
  total = 0;
  if(level == 1){
    vb1 = 0; db1 = 0;
  } else if(level == 2){
    vbL2_0 = 0; vbL2_1 = 0; dbL2_1 = 0; dbL2_0 = 0;
  } else if(level == 3){
    vbL3_0 = 0; vbL3_1 = 0; vbL3_2 = 0; vbL3_3 = 0; vbL3_4 = 0; vbL3_5 = 0; vbL3_6 = 0; vbL3_7 = 0;
    dbL3_0 = 0; dbL3_1 = 0; dbL3_2 = 0; dbL3_3 = 0; dbL3_4 = 0; dbL3_5 = 0; dbL3_6 = 0; dbL3_7 = 0;    
  }
}

void pressed_start(){
  digitalWrite(LED_READY, LOW);
  state = 1;
  Serial.write(packet_SYN_0);
  Serial.write(packet_SYN_1);
  delay_flag = 0;  
}

void pressed_delay(){
  delay_flag = !delay_flag;
  if(delay_flag == true){
    digitalWrite(LED_DELAY, HIGH);
  } else {
    digitalWrite(LED_DELAY, LOW);
  }
}

void delayed(){
  if(delay_flag == true){
    delay(1000);
  }
}

void show_total(int total){
  // show total load/store operations
  // separate tens and ones digit
  int tens = (total / 10) % 10;
  int ones = total % 10;

  // convert tens to binary, then send to decoder A / left decoder
  if(tens == 0){
    digitalWrite(SEG_A3, 0); digitalWrite(SEG_A2, 0); digitalWrite(SEG_A1, 0); digitalWrite(SEG_A0, 0);
  } else if(tens == 1){
    digitalWrite(SEG_A3, 0); digitalWrite(SEG_A2, 0); digitalWrite(SEG_A1, 0); digitalWrite(SEG_A0, 1);
  } else if(tens == 2){
    digitalWrite(SEG_A3, 0); digitalWrite(SEG_A2, 0); digitalWrite(SEG_A1, 1); digitalWrite(SEG_A0, 0);
  } else if(tens == 3){
    digitalWrite(SEG_A3, 0); digitalWrite(SEG_A2, 0); digitalWrite(SEG_A1, 1); digitalWrite(SEG_A0, 1);
  } else if(tens == 4){
    digitalWrite(SEG_A3, 0); digitalWrite(SEG_A2, 1); digitalWrite(SEG_A1, 0); digitalWrite(SEG_A0, 0);
  } else if(tens == 5){
    digitalWrite(SEG_A3, 0); digitalWrite(SEG_A2, 1); digitalWrite(SEG_A1, 0); digitalWrite(SEG_A0, 1);
  } else if(tens == 6){
    digitalWrite(SEG_A3, 0); digitalWrite(SEG_A2, 1); digitalWrite(SEG_A1, 1); digitalWrite(SEG_A0, 0);
  } else if(tens == 7){
    digitalWrite(SEG_A3, 0); digitalWrite(SEG_A2, 1); digitalWrite(SEG_A1, 1); digitalWrite(SEG_A0, 1);
  } else if(tens == 8){
    digitalWrite(SEG_A3, 1); digitalWrite(SEG_A2, 0); digitalWrite(SEG_A1, 0); digitalWrite(SEG_A0, 0);
  } else if(tens == 9){
    digitalWrite(SEG_A3, 1); digitalWrite(SEG_A2, 0); digitalWrite(SEG_A1, 0); digitalWrite(SEG_A0, 1);
  }

  
  // convert ones to binary, then send to decoder B / right decoder
  if(ones == 0){
    digitalWrite(SEG_B3, 0); digitalWrite(SEG_B2, 0); digitalWrite(SEG_B1, 0); digitalWrite(SEG_B0, 0);
  } else if(ones == 1){
    digitalWrite(SEG_B3, 0); digitalWrite(SEG_B2, 0); digitalWrite(SEG_B1, 0); digitalWrite(SEG_B0, 1);
  } else if(ones == 2){
    digitalWrite(SEG_B3, 0); digitalWrite(SEG_B2, 0); digitalWrite(SEG_B1, 1); digitalWrite(SEG_B0, 0);
  } else if(ones == 3){
    digitalWrite(SEG_B3, 0); digitalWrite(SEG_B2, 0); digitalWrite(SEG_B1, 1); digitalWrite(SEG_B0, 1);
  } else if(ones == 4){
    digitalWrite(SEG_B3, 0); digitalWrite(SEG_B2, 1); digitalWrite(SEG_B1, 0); digitalWrite(SEG_B0, 0);
  } else if(ones == 5){
    digitalWrite(SEG_B3, 0); digitalWrite(SEG_B2, 1); digitalWrite(SEG_B1, 0); digitalWrite(SEG_B0, 1);
  } else if(ones == 6){
    digitalWrite(SEG_B3, 0); digitalWrite(SEG_B2, 1); digitalWrite(SEG_B1, 1); digitalWrite(SEG_B0, 0);
  } else if(ones == 7){
    digitalWrite(SEG_B3, 0); digitalWrite(SEG_B2, 1); digitalWrite(SEG_B1, 1); digitalWrite(SEG_B0, 1);
  } else if(ones == 8){
    digitalWrite(SEG_B3, 1); digitalWrite(SEG_B2, 0); digitalWrite(SEG_B1, 0); digitalWrite(SEG_B0, 0);
  } else if(ones == 9){
    digitalWrite(SEG_B3, 1); digitalWrite(SEG_B2, 0); digitalWrite(SEG_B1, 0); digitalWrite(SEG_B0, 1);
  }
}
