;
;-----------------------------------------
; 16/8 signed Divide
;
;	short div16x8s(short dividend, signed char divisor);
;
; PASSED:       dividend = r25:r24, divisor = r23
; RETURNED:     results r25:r24
; USED:         R1 and T
; STACK:        2
;
	seg flash.code.f_div16x8s
	
public _div16x8s:
	push	r22
        mov     R1, r25
        eor     R1, r23		; store XORd dividend and divisor 
        bst     R1, 7		; sign bits in T
        ldi     R22, 17		; Set up loop counter
        mov     R1, R22
        push    r23
        
        sbrs    r25,7    ; X = ABS(X)
        rjmp    d16s1
        com     r24
        com     r25
        adiw    r24, 1
d16s1:
        sbrc    r23,7    ; Wl = ABS(Wl)
        neg     r23

        sub     R22, R22	        ;clear remainder word and carry
        ;
        ; Now, start to do the actual divide!
        ;
d16s_3:
        rol     r24
        rol     r25
        
        dec     R1
        breq    d16sxx

        rol     r22		; shift dividend into remainder
        sub     R22,r23		; remainder = remainder - divisor
        brcc    d16s_6          ; if result negative
        add     R22, r23         ;    restore remainder
        clc                     ;    clear carry to be shifted into result
        rjmp    d16s_3          ; else
d16s_6:
        sec                     ;    set carry to be shifted into result
        rjmp    d16s_3
d16sxx:
        brtc    d16s_4          ; If sign bit (T) set
        com     r24
        com     r25
        adiw    r24, 1
d16s_4:
        pop     r23             ;    Restore original Xl
	pop	r22
        ret
