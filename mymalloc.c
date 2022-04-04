#include "mymalloc.h"

#define DOBI_16_BITNI_NASLOV(X) (__uint16_t)((uintptr_t)(X)&0XFFF) 
#define DOBI_CELI_NASLOV(X,Y) (__uint16_t*)((uintptr_t)(X)|(Y))
#define TRENUTNA_STRAN(X) (glava*)((uintptr_t)(X)&~0XFFF)

glava *prvi_kazalec = NULL;

 size_t naslednja_potenca2(size_t stevilo)
{
    stevilo -= 1;
	stevilo |= (stevilo >> 1);
	stevilo |= (stevilo >> 2);
	stevilo |= (stevilo >> 4);
	stevilo |= (stevilo >> 8);
    stevilo |=(stevilo >> 16);
    stevilo |=(stevilo >> 32);
    return stevilo+1;
}

int dodaj_stran(glava *prejsni)
{
  //  printf("Dodajam stran\n");
    long velikost_strani = sysconf(_SC_PAGESIZE); 
    __uint8_t *tmp = (__uint8_t*)mmap(NULL, velikost_strani, PROT_READ|PROT_WRITE, MAP_ANONYMOUS|MAP_PRIVATE, -1, 0);
    if (tmp == MAP_FAILED) return -1;
    glava gl;
    gl.naslednji_segment = NULL;
    __uint16_t *prosti = (__uint16_t*)(tmp + sizeof(glava));
    gl.prosti = DOBI_16_BITNI_NASLOV(prosti);
    *((glava*)(tmp)) = gl;
    *(prosti) = (velikost_strani-sizeof(glava));
    *(prosti+1) = 0; 
    *(prosti+2) = 0;
    *((uint16_t*)(tmp+velikost_strani-2))= (velikost_strani-sizeof(glava));
    if (prejsni)prejsni->naslednji_segment = (glava*)tmp;
    else prvi_kazalec = (glava*)tmp;
    return 0;
}

void razcepi(__uint16_t *ptr, __uint16_t nova_velikost)
{
    glava* stran = TRENUTNA_STRAN(ptr);
    __uint16_t velikost = *ptr;
    __uint16_t nalslednji = *(ptr+1);
    __uint16_t prejsni = *(ptr+2);
    __uint16_t *p = DOBI_CELI_NASLOV(stran,prejsni);
    __uint16_t *n = DOBI_CELI_NASLOV(stran,nalslednji);

    if(nova_velikost>(velikost-MIN_VELIKOST)){
        if(!prejsni){
            stran->prosti = nalslednji;
        }else{
            *(p+1) = nalslednji;
        }
        if(nalslednji){
            *(n+2) = prejsni;
        }
    }else{
        *(ptr+nova_velikost/2) = velikost-nova_velikost;
        *(ptr+velikost/2-1) = velikost-nova_velikost;
        *(ptr+nova_velikost/2+1) = nalslednji;
        *(ptr+nova_velikost/2+2) = prejsni;

        *ptr = nova_velikost|1;
        *(ptr+nova_velikost/2-1) = nova_velikost|1;

        if(!prejsni){
            stran->prosti = DOBI_16_BITNI_NASLOV(ptr+nova_velikost/2);
        }else{
            *(p+1) =  DOBI_16_BITNI_NASLOV(ptr+nova_velikost/2);           
        }
        if (nalslednji){
            *(n+2) = DOBI_16_BITNI_NASLOV(ptr+nova_velikost/2); 
        }
    }
}

void brisi_stran(glava *stran,long velikost_strani)
{
      //puts("Brisem stran\n");
        if(prvi_kazalec == stran){
            prvi_kazalec = stran->naslednji_segment;
             munmap(stran,velikost_strani);
             return ;
        }

        glava *cnt = prvi_kazalec;
        while(cnt->naslednji_segment!=stran){
            cnt = cnt->naslednji_segment;

        }
        cnt->naslednji_segment = stran->naslednji_segment;
        munmap(stran,velikost_strani);
        return ;

}




int zbrisi(__uint16_t *ptr,long velikost_strani)
{
    if(!(*ptr&1)){
        return 0;
    }
    glava* stran = TRENUTNA_STRAN(ptr);
    __uint16_t velikost = *ptr&-2;

    if(velikost == velikost_strani-sizeof(glava)){
        brisi_stran(stran,velikost_strani);
        return 1;   
    }

    if(!(stran->prosti)){
        *ptr = velikost;
        *(ptr+velikost/2-1) = velikost;
        *(ptr+1) = 0;
        *(ptr+2) = 0;
        stran->prosti = DOBI_16_BITNI_NASLOV(ptr);
        return 1; 
    }
    __uint16_t *prvi = DOBI_CELI_NASLOV(stran,stran->prosti);
    __uint16_t next = stran->prosti;
    __uint16_t prv = stran->prosti;
    while(next){
       // printf("%p %d\n",prvi,*prvi);
        if(ptr<prvi){
         //   printf("Vstavljam kazalec %p, prvi je: %p\n",ptr,prvi);
            __uint16_t prejsni =*(prvi+2);
            if(!prejsni){
              //  puts("Vstavljam na zacetek\n");
                stran->prosti = DOBI_16_BITNI_NASLOV(ptr);
                *(ptr+1) = next;
                *(ptr+2) = 0;
                *(prvi+2) = DOBI_16_BITNI_NASLOV(ptr);
                __uint16_t nova_velikost = velikost;
                if(!((*(ptr+velikost/2))&1 )){
                  //  puts("Zdruzujem z nasledjnim...\n");
                    nova_velikost+=(*(ptr+velikost/2));
                    if(nova_velikost == velikost_strani-sizeof(glava)){
                        brisi_stran(stran,velikost_strani);
                        return 1;
                    }
                    __uint16_t naprej = *(ptr+velikost/2+1);
                    *(ptr+1) = naprej;
                    if(naprej){
                     __uint16_t *n = DOBI_CELI_NASLOV(stran,naprej);
                     *(n+2) = DOBI_16_BITNI_NASLOV(ptr);
                    }
              
                } 

                *ptr = nova_velikost;
                *(ptr+nova_velikost/2-1) = nova_velikost;
            
           }else{
            //puts("Vstavljam na sredino\n");
            __uint16_t p = *(prvi+2);

            *ptr = velikost;
            *(ptr+velikost/2-1) = velikost;
            *(ptr+1) = next;
            *(ptr+2) = p;
            __uint16_t* prej = DOBI_CELI_NASLOV(stran,p);
            *(prej+1)=DOBI_16_BITNI_NASLOV(ptr);
            *(prvi+2) = DOBI_16_BITNI_NASLOV(ptr);
            __uint16_t nova_velikost = velikost;

             if(!((*(ptr+velikost/2))&1 )){
                 //puts("Zdruzujem z naslednjim...\n");
                 nova_velikost += *prvi;
                 *(ptr) = nova_velikost;
                 *(ptr+nova_velikost/2-1) = nova_velikost;

                 *(ptr+1) = *(prvi+1);
                 next = *(ptr+1);

                 if(*(prvi+1)){
                     __uint16_t naprej = *(prvi+1);
                    *(DOBI_CELI_NASLOV(stran,naprej)+2) = DOBI_16_BITNI_NASLOV(ptr);
                 }            
             }
             if(!((*(ptr-1))&1)){
                 // puts("Zdruzujem z prejsnim...\n");
                  __uint16_t vv = *(ptr-1);
                  
                //  printf("%d\n",*(prvi+1));
                 nova_velikost +=vv;
                 if(nova_velikost == velikost_strani-sizeof(glava)){
                        brisi_stran(stran,velikost_strani);
                        return 1;
                    }
                 *(ptr-vv/2) = nova_velikost;
                 *(ptr-vv/2+1) = next;
                 *(ptr-vv/2+nova_velikost/2-1) = nova_velikost;
                
                  
                    if(next){
                     __uint16_t *u = DOBI_CELI_NASLOV(stran,next);
                     *(u+2) = DOBI_16_BITNI_NASLOV(ptr-vv/2);
                    }
             }
             
         }
     
             return 1;
        }
        prv = next;
        next = *(prvi+1);
       prvi = DOBI_CELI_NASLOV(stran,next);

    }
    
    
         //puts("Vstavljam na konec\n");
            __uint16_t nova_velikost = velikost;
            *(ptr) = velikost;
            *(ptr+velikost/2-1) = velikost;
            *(ptr+2) = prv;
            *(ptr+1) = 0;
          if(!((*(ptr-1))&1)){
                 // puts("Zdruzujem z prejsnim...\n");
                  __uint16_t vv = *(ptr-1);
                 nova_velikost +=vv;
                 if(nova_velikost == velikost_strani-sizeof(glava)){
                        brisi_stran(stran,velikost_strani);
                        return 1;
                    }
                 *(ptr-vv/2) = nova_velikost;
                 *(ptr-vv/2+1) = 0;
                 *(ptr-vv/2+nova_velikost/2-1) = nova_velikost;

               
             }

}


    


__uint16_t* najdi(__uint16_t velikost)
{
    if(!prvi_kazalec)  dodaj_stran(NULL);


    glava *it = prvi_kazalec;
    glava *pp = it;
    __uint16_t *najbolsi = NULL;
while(it){
    if(!it->prosti){
    pp=it;
    it = it->naslednji_segment;
    continue;

    }
    __uint16_t *prvi = DOBI_CELI_NASLOV(it,it->prosti);
    __uint16_t next = prvi_kazalec->prosti;
    __uint16_t prv = next;


    while(next){
        //printf("Naslov: %p, velikost: %d, naslednji %d, prejsni %d, konec %d \n",prvi,*(prvi),*(prvi+1),*(prvi+2),*(prvi+*(prvi)/2-1));
        if(*prvi>=velikost){
            if(!najbolsi) najbolsi=prvi;
            if(*prvi<*najbolsi) najbolsi = prvi;
        }
        prv = next;      
        next = *(prvi+1);
        prvi = DOBI_CELI_NASLOV(prvi_kazalec,next);
    }
    pp=it;
    it = it->naslednji_segment;
    }
    if(!najbolsi){
        dodaj_stran(pp);
        pp=pp->naslednji_segment;
        najbolsi = DOBI_CELI_NASLOV(pp,pp->prosti);
    }
    razcepi(najbolsi,velikost);
    //puts("");
    return najbolsi;
}


void *mymalloc(size_t size)
{
    if(!size) return NULL;
    long velikost_strani = sysconf(_SC_PAGESIZE); 

    size_t velikost =size+4;
    velikost = naslednja_potenca2(velikost);
    if(velikost>velikost_strani-sizeof(glava))
    {
      //printf("VELIKA STRAAAN\n");
         velikost = size+sizeof(size_t)+2;
        size_t v = velikost%velikost_strani;
        if(v>0) velikost +=velikost_strani-v;
        size_t*podatki =  (size_t*)mmap(NULL,velikost,PROT_WRITE|PROT_READ,MAP_ANONYMOUS|MAP_PRIVATE,0,0);
        if (podatki == MAP_FAILED) return NULL;
       *podatki=velikost;
       __uint16_t* kk = (__uint16_t*)podatki;
       *(kk+sizeof(size_t)/2) = __UINT16_MAX__;
       kk = kk+sizeof(size_t)/2+1;
        return (void*)kk;    
    }

    __uint16_t *ptr = najdi(velikost);
  

 return (void*)(ptr+1);

}
void myfree(void *ptr)
{
    if(!ptr) return;
    __uint16_t *p = (__uint16_t*)(ptr);
     long velikost_strani = sysconf(_SC_PAGESIZE); 
     if (*(p-1)==__UINT16_MAX__){
         p = p-1;
         size_t* uu = (size_t*)(p-sizeof(size_t)/2);
         size_t velikost = *uu;
        // printf("Velikost velike strani %d\n",velikost);
         munmap(uu,velikost);
         return;
     }
   
    zbrisi((p-1),velikost_strani);

}