#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include <semaphore.h>
#include <string.h>
#include <unistd.h>

//compile with gcc -g -pthread name.c -o otherName
//also -pthread must be added to the args seciton of tasks.json

//Variables taken from assignment file
// The number of registration desks that are available.
int REGISTRATION_SIZE = 10;
// The number of restrooms that are available.
int RESTROOM_SIZE = 10;
//The number of cashiers in cafe that are available.
int CAFE_NUMBER = 10;
// The number of General Practitioner that are available.
int GP_NUMBER = 10;
// The number of cashiers in pharmacy that are available.
int PHARMACY_NUMBER = 10;
// The number of assistants in blood lab that are available.
int BLOOD_LAB_NUMBER = 10;
// The number of operating rooms, surgeons and nurses that are available.
int OR_NUMBER = 10;
int SURGEON_NUMBER = 30;
int NURSE_NUMBER = 30;
/* The maximum number of surgeons and nurses that can do a surgery. A random value is
calculated for each operation between 1 and given values to determine the required
number of surgeons and nurses.*/
int SURGEON_LIMIT = 5;
int NURSE_LIMIT = 5;
// The number of patients that will be generated over the course of this program.
int PATIENT_NUMBER = 1000;
// The account of hospital where the money acquired from patients are stored.
int HOSPITAL_WALLET = 0;
/* The time required for each operation in hospital. They are given in milliseconds. But
you must use a randomly generated value between 1 and given values below to determine
the time that will be required for that operation individually. This will increase the
randomness of your simulation.
The WAIT_TIME is the limit for randomly selected time between 1 and the given value
that determines how long a patient will wait before each operation to retry to execute.
Assuming the given department is full*/
int ARRIVAL_TIME = 100;
int WAIT_TIME = 100;
int REGISTRATION_TIME = 100;
int GP_TIME = 200;
int PHARMACY_TIME = 100;
int BLOOD_LAB_TIME = 200;
int SURGERY_TIME = 500;
int CAFE_TIME = 100;
int RESTROOM_TIME = 100;
/* The money that will be charged to the patients for given operations. The registration
and blood lab costs should be static (not randomly decided) but pharmacy and cafe cost
should be randomly generated between 1 and given value below to account for different
medicine and food that can be purchased.
The surgery cost should calculated based on number of doctors and nurses that was
required to perform it. The formula used for this should be:
SURGERY_OR_COST + (number of surgeons * SURGERY_SURGEON_COST) +
(number of nurses * SURGERY_NURSE_COST)*/
int REGISTRATION_COST = 100;
int PHARMACY_COST = 200; // Calculated randomly between 1 and given value.
int BLOOD_LAB_COST = 200;
int SURGERY_OR_COST = 200;
int SURGERY_SURGEON_COST = 100;
int SURGERY_NURSE_COST = 50;
int CAFE_COST = 200; // Calculated randomly between 1 and given value.
// The global increase rate of hunger and restroom needs of patients. It will increase
//randomly between 1 and given rate below.
int HUNGER_INCREASE_RATE = 10;
int RESTROOM_INCREASE_RATE = 10;
//Patients limit of their need. When this limits are passed they have to use restroom or cafe respectively
int RESTROOM_LIMIT = 100;
int HUNGER_LIMIT = 100;




typedef struct patientStruct{
    //Using a patient struct to keep track of their needs
    int no, Hunger_Meter, Restroom_Meter;
} Patient;

sem_t registiration;
sem_t restroom;
sem_t cafe;
sem_t gp;
sem_t pharmacy;
sem_t blood_lab;
sem_t surgeon;
sem_t nurse;
sem_t or;
//I made all of them semaphore because they all have more than one so at a time more than one person can use them
//also they do not change the value of a variable anywhere so a lock is not needed


pthread_mutex_t walletLock;
//Will be using a lock when chancing the value of hospital_wallet because when different threads try to change it some of the value will be lost
//so with the use of lock that problem will not accur

void addToWallet(int* addMoney){
    pthread_mutex_lock(&walletLock);
    HOSPITAL_WALLET += *addMoney;
    pthread_mutex_unlock(&walletLock);
}

void waitToTry(){
    //Waits a random amount of time between 1 and WAIT_TIME 
    //Used it as a function to decrease the repeated lines
    int waitTime = (rand() % WAIT_TIME) + 1;
    usleep(waitTime * 1000);
}

void waiting(Patient* p, Patient* out){
    //Increases the hunger and toilet paramaters of the patient while they are waiting in a line
    //Does not called while they are waiting in line at the cafe or restroom to prevent infinite loops of going to one after another
    int randomRestroomIncrease = (rand() % RESTROOM_INCREASE_RATE) + 1;//Increasing the needs of the patient randomly between 1 and their increase rates
    int randomHungerIncrease = (rand() % HUNGER_INCREASE_RATE) + 1;
    out->Restroom_Meter += randomRestroomIncrease;//Randomly selected number is added
    out->Hunger_Meter += randomHungerIncrease;
    if (out->Hunger_Meter >= HUNGER_LIMIT){
        //Limit of the need is passed
        printf("%d. patient goes to the cafe. Waits an empty register to use.\n", out->no);
        sem_wait(&cafe);
        printf("%d patient uses a cafe register.\n", out->no);
        int cafeTime = (rand() % CAFE_TIME + 1);
        usleep(cafeTime * 1000);//Uses cafe
        out->Hunger_Meter = 0;//Patient has no hunger need now
        sem_post(&cafe);
        //A patient has used a cafe register
        //A random amount of money between 1 and CAFE_COST is added to the hospital wallet
        int spentMoney = (rand() %CAFE_COST) + 1;
        addToWallet(&spentMoney);
    }
    if (out->Restroom_Meter >= RESTROOM_LIMIT){
        //Limit of the need is passed
        printf("%d. patient goes to the restroom. Waits an empty restroom to use.\n", out->no);
        sem_wait(&restroom);
        printf("%d patient uses a restroom.\n", out->no);
        int restroomTime = (rand() % RESTROOM_TIME) + 1;
        usleep(restroomTime * 1000);//Uses restroom
        out->Restroom_Meter = 0;//Patient has no restroom need now
        sem_post(&restroom);
    }
}

void pharmacyLine(Patient* nPatient){
    //Patient is at the pharmacy and waits to buy prescribed medicine
   printf("%d. patient is prescribed medicine. Waits in line at pharmacy.\n", nPatient->no);
   while(1){
       if (sem_trywait(&pharmacy) == 0){
           //Tries to find a open register
           break;
       }
       waiting(nPatient, nPatient);
       waitToTry();//Waits for the next try
   }
   int pharmacyTime = (rand() % PHARMACY_TIME) + 1;
   usleep(pharmacyTime * 1000);//Buys prescribed medicine at pharmacy
   sem_post(&pharmacy);
   int spentMoney = (rand() % PHARMACY_COST) + 1;//Randomly selected spent money is decided
   addToWallet(&spentMoney);//Spent money at the pharmacy is added to the hospital wallet
   printf("%d. patient is bought the prescribed medicine. Returns home.\n", nPatient->no);//Patients returns home
}

void surgeryLine(Patient* nPatient){
    //Patient is at the surgery and waits at the surgery line
    printf("%d. patient need to have a surgery. Waits in line for surgery.\n", nPatient->no);
    while(1){
        //Tries to find an empty operation room
        if (sem_trywait(&or) == 0)
        {
            break;
        }
        waiting(nPatient, nPatient);
        waitToTry();
    }
    int surgeonNo = (rand() % SURGEON_LIMIT) + 1;//Randomly decided required surgeons for the operation
    printf("%d. waits the correct amount of surgeons.\n", nPatient->no);    
    while(1){
        //firstly tries to get the required number of surgeons
        int count = 0, flag = 1;//Flag 0 is false flag 1 is true
        for(int i = 0; i < surgeonNo; i++){
            if(sem_trywait(&surgeon) == 0){
                //Count stores the succesfully got surgeons
                count++;
            } else {
                //If it fails to get any of the required surgeons than it has to do the waiting parts
                //so it goes out of this for loop unsuccessfully
                flag = 0;
                break;
            }
        }
        if(flag){
            //All of the surgeons are successfully got
            break;
        } else {
            for(int i = count; i  > 0; i--){
                //It cant get enough surgeons so it gives back the gotten surgeons to prevent deadlocks
                sem_post(&surgeon);
            }
        }
        //Not enough surgeons now so it must wait
        waiting(nPatient, nPatient);
        waitToTry();
    }
    //Below is the same as surgeons just repeated for nurses
    int nurseNo = (rand() % NURSE_LIMIT) + 1;
    printf("%d. patient waits the correct amount of nurses.\n", nPatient->no);
    while(1){
        //Lastly it tries to get required numbber of nurses
        int count = 0, flag = 1;
        for(int i = 0; i < nurseNo; i++){
            if(sem_trywait(&nurse) == 0){
                count++;
            } else {
                flag = 0;
            }
        }
        if(flag){
            break;
        } else {
            for(int i = count; i  > 0; i--){
                sem_post(&nurse);
            }
        }
        waiting(nPatient, nPatient);
        waitToTry();
    }
    //All of the required operators got so now surgery is done
    int surgeryTime = (rand() % SURGERY_TIME) + 1;
    usleep(surgeryTime* 1000);//Surgery is done
    //Semaphores are given back in the order they are taken to make program run faster
    sem_post(&or);
    for(int i = surgeonNo; i > 0; i--){
        sem_post(&surgeon);
    }
    for(int i = nurseNo; i > 0; i--){
        sem_post(&nurse);
    }
    int spentMoney = (SURGERY_OR_COST + (surgeonNo * SURGERY_SURGEON_COST) + (nurseNo * SURGERY_NURSE_COST));
    addToWallet(&spentMoney);
    printf("%d. patient had surgery. Returns to general practitioner to have a check up.\n", nPatient->no);
}

void* patient(void* num){
    Patient nPatient = {
        //Used a struct to manage everything easily
        .no = *(int*)num,
        .Hunger_Meter = (rand() % HUNGER_LIMIT) + 1,//Giving their needs randomly to make it realistic
        .Restroom_Meter  = (rand() % RESTROOM_LIMIT) + 1
    };
    printf("%d. patient goes to the hospital. Waits in line for an open registery.\n", nPatient.no);//Patient is at the hospital
    while(1){
        //Tries to find a empty registery at the refgistiration
        if (sem_trywait(&registiration) == 0){
            break;
        }
        waiting(&nPatient, &nPatient);
        waitToTry();
    }
    printf("%d. patient entered the registery.\n", nPatient.no);
    int randomTime = (rand() % REGISTRATION_TIME) + 1;
    usleep(randomTime * 1000);//Uses registery to register
    sem_post(&registiration);
    addToWallet(&REGISTRATION_COST);//Registery cost is added to the hospital wallet
    printf("%d. patient waits to examined by General Practitioner.\n", nPatient.no);
    while(1){
        //Tries to find an empty practitioner
        if (sem_trywait(&gp) == 0)
        {
            break;
        }
        waiting(&nPatient, &nPatient);
        waitToTry();
    }
    randomTime = (rand() % GP_TIME) + 1;
    usleep(randomTime * 1000);//Examined by gp
    sem_post(&gp);
    int disease = (rand() % 3) + 1;// pharmacy, blood test or surgery its randomly decided
    if (disease == 1)
    {
        //Patient is prescribed medicine and they go to the pharmacy to buy them
        pharmacyLine(&nPatient);
    } else if(disease == 2){
        //Patient is asked for a blood test and goes to labs to give blood sample
        printf("%d. patient is asked for blood test. Waits in line at blood lab.\n", nPatient.no);
        while(1){
            //Tries to find an empty lab
            if (sem_trywait(&blood_lab) == 0)
            {
                break;
            }
            waiting(&nPatient, &nPatient);
            waitToTry();
        }
        randomTime = (rand() % BLOOD_LAB_TIME) + 1;
        usleep(randomTime * 1000);//Gives blood at the blood lab
        sem_post(&blood_lab);
        addToWallet(&BLOOD_LAB_COST);//Money for taking blood is added to the hospital wallet
        printf("%d. patient gave blood at the lab. Returns to the Practitioner for their treatment.\n", nPatient.no);
        //Patient gave blood sample
        while(1){
            //Tries to find an empty practitioner
            if (sem_trywait(&gp) == 0)
            {
                break;
            }
            waiting(&nPatient, &nPatient);
            waitToTry();
        }
        randomTime = (rand() % GP_TIME) + 1;
        usleep(randomTime * 1000);//Examined by gp
        sem_post(&gp);
        int disease = (rand() % 2) + 1;// pharmacy, or no pharmacy
        if (disease == 1){
            //Paitent needs medicine so they go to the pharmacy to buy them
            pharmacyLine(&nPatient);
        } else{
            //Patient might not need any medicine so they return home
            printf("%d. patient does not need any medicine. Returns home.\n", nPatient.no);
        }
    } else{
        //Patient needs surgery so they go to the surgery
        surgeryLine(&nPatient);
        while(1){
            //After the surgery is done patient must have a check up at the practitioner
            //Tries to find an empty practitioner
            if (sem_trywait(&gp) == 0)
            {
                break;
            }
            waiting(&nPatient, &nPatient);
            waitToTry();
        }
        randomTime = (rand() % GP_TIME) + 1;
        usleep(randomTime * 1000);//Examined by gp
        sem_post(&gp);
        int disease = (rand() % 2) + 1;// pharmacy, or no pharmacy
        if (disease == 1){
            //Paitent needs medicine so they go to the pharmacy to buy them
            pharmacyLine(&nPatient);
        } else{
            //Patient might not need any medicine
            printf("%d. patient does not need any medicine. Returns home.\n", nPatient.no);
        }
    }
    free(num);
}

int main () {
    srand(time(NULL));//Setting random number generator
    pthread_mutex_init(&walletLock, NULL); //Setting the mutex
    //Setting semaphors
    sem_init(&registiration, 0, REGISTRATION_SIZE);
    sem_init(&restroom, 0, RESTROOM_SIZE);
    sem_init(&cafe, 0, CAFE_NUMBER);
    sem_init(&gp, 0, GP_NUMBER);
    sem_init(&pharmacy, 0, PHARMACY_NUMBER);
    sem_init(&blood_lab, 0, BLOOD_LAB_NUMBER);
    sem_init(&surgeon, 0, SURGEON_NUMBER);
    sem_init(&nurse, 0, NURSE_NUMBER);
    sem_init(&or, 0, OR_NUMBER);
    pthread_t patients[PATIENT_NUMBER];//Patients are threads
    int i;
    for(i = 0; i < PATIENT_NUMBER; i++){
        int* number = malloc(sizeof(int));
        *number = i;
        if(pthread_create(&patients[i], NULL, &patient, (void*) number) != 0){
            perror("Failed to create thread\n");
        }
        //Waits a random amount of time after every patient is created to make it more realistic
        int randomTime = (rand() % ARRIVAL_TIME) + 1;
        usleep(randomTime * 1000);
    }
    for(int i = 0; i < PATIENT_NUMBER; i++){
        //Joins every thread so program waits all of them to finish
        if(pthread_join(patients[i], NULL) != 0){
            perror("Failed to create thread\n");
        }
    }
    printf("Hospital has made %d$\n", HOSPITAL_WALLET);
    pthread_mutex_destroy(&walletLock);
    //Destroying the semaphors
    sem_destroy(&registiration);
    sem_destroy(&restroom);
    sem_destroy(&cafe);
    sem_destroy(&gp);
    sem_destroy(&pharmacy);
    sem_destroy(&blood_lab);
    sem_destroy(&surgeon);
    sem_destroy(&nurse);
    sem_destroy(&or);
}