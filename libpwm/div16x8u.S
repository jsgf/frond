;
;-----------------------------------------
; 16/8 unsigned Divide
;
;	unsigned short div16x8s(unsigned short dividend, unsigned char divisor);
;
; PASSED:       dividend = r25:r24, divisor = r23
; RETURNED:     results r25:r24
; USED:         R1 and T
; STACK:        2
;
	seg flash.code.f_div16x8u

public _div16x8u:
	push	r22
        ldi     R22, 17		; Set up loop counter
        mov     R1, R22
        push    r23
        
        sub     R22, R22	;clear remainder word and carry
        ;
        ; Now, start to do the actual divide!
        ;
d16u_loop:
        rol     r24
        rol     r25
        
        dec     R1
        breq    d16u_done

        rol     r22		; shift dividend into remainder
        sub     R22,r23		; remainder = remainder - divisor
        brcc    d16u_pos	; if result negative
        add     R22, r23	;    restore remainder
        clc			;    clear carry to be shifted into result
        rjmp    d16u_loop	; else
d16u_pos:
        sec                     ;    set carry to be shifted into result
        rjmp    d16u_loop
d16u_done:
        pop     r23
	pop	r22
        ret
