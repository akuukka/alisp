namespace alisp { inline const char* getInitCode() { return R"code(

(defun caar (cons-cell)
  (car (car cons-cell)))

(defun cadr (cons-cell)
  (nth 1 cons-cell))

(defun cdar (cons-cell)
  (cdr (car cons-cell)))

(defun car-safe (object)
  (let ((x object))
    (if (consp x)
        (car x)  
      nil)))

(defun cddr (cons-cell)
  (cdr (cdr cons-cell)))

(defun cdddr (cons-cell)
  (cdr (cdr (cdr cons-cell))))

(defun cdr-safe (object)
  (let ((x object))
    (if (consp x)
        (cdr x)
      nil)))

(defmacro pop (listname)
  (list 'prog1 (list 'car listname)
        (list 'setq listname 
              (list 'cdr listname))))

(defmacro push (element listname)
  (list 'setq listname
        (list 'cons element listname)))

(defmacro setq (sym var)
  (list 'set (list 'quote sym) var))

; Yes, this setf for (car x) is incredibly ugly but it's a decent start!
(defmacro setf (var value)
  (let ((li (list 'setq var value)))
    (if (eq 'car (car-safe var))
        (setq li (list 'let* (list (list 'v (cadr var))) (list 'setcar 'v value)  )))
    (if (eq 'cdr (car-safe var))
        (setq li (list 'let* (list (list 'v (cadr var))) (list 'setcdr 'v value)  )))
    (if (eq 'cadr (car-safe var))
        (setq li (list 'let* (list (list 'v (cadr var))) (list 'setcar (list 'cdr 'v) value)  )))
    li))

(defun getf(plist indicator &optional default)
  (let ((ret default) (found nil) )
    (while (and (cdr plist) (eq found nil) )
      (if (eq (car plist) indicator)
          (progn
            (setq ret (cadr plist))
            (setq found t)))
      (setq plist (cddr plist)))
    ret
    ))

(defun get (symbol property)
  "Returns the value of the property named property in symbol’s property list."
  (getf (symbol-plist symbol) property))

(defmacro when (cond &rest body)
  "If COND yields non-nil, do BODY, else return nil."
  (list 'if cond (cons 'progn body)))

(defmacro lambda (&rest cdr)
  "Return an anonymous function."
  (list 'function (cons 'lambda cdr)))

(defun indirect-function (function)
  (if (symbolp function)
      (indirect-function (symbol-function function))
    function))

(defun error (&rest args)
  (signal 'error (list (apply 'format args))))

(defun define-error (name message &optional parent)
  (if (not parent)
      (setq parent 'error))
  (put name 'error-conditions (append (list name) (get parent 'error-conditions)))
  (put name 'error-message message)
  message)

(put 'error 'error-conditions '(error))
(define-error 'arith-error "Arithmetic error")
(define-error 'wrong-type-argument "Wrong type argument")

)code"; }}
