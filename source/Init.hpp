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

)code"; }}
