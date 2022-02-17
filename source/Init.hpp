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

(defmacro defsetf (access-fn update-fn)
  "Add as simple setf rule"
  (list 'put (list 'quote 'setf-simple-rules) (list 'quote access-fn) (list 'quote update-fn)))

(defsetf car setcar)
(defsetf cdr setcdr)
(defsetf symbol-value set)

(defmacro setf (var value)
  (let ((li (list 'setq var value))
        (simple (get 'setf-simple-rules (car-safe var))))
    (when simple
      (let ((params (append (cdr var) (list value))))
        (setq li `(apply ',simple (list ,@params)))))
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

(defmacro unless (cond &rest body)
  "If COND yields nil, do BODY, else return nil."
  (cons 'if (cons cond (cons nil body))))

(defmacro lambda (&rest cdr)
  "Return an anonymous function."
  (list 'function (cons 'lambda cdr)))

(defun indirect-function (function)
  (if (symbolp function)
      (indirect-function (symbol-function function))
    function))

(defun remq (elt list)
  (while (and (eq elt (car list)) (setq list (cdr list))))
  (if (memq elt list)
      (delq elt (copy-sequence list))
    list))

(defun seq-elt (sequence n)
  "Return Nth element of SEQUENCE."
  (elt sequence n))

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
(define-error 'circular-list "Circular list")
(define-error 'wrong-type-argument "Wrong type argument")
(define-error 'void-function "Void function")
(define-error 'void-variable "Void variable")
(define-error 'invalid-function "Invalid function")
(define-error 'setting-costant "Setting constant")
(define-error 'wrong-number-of-arguments "Wrong number of arguments")

(defvar gensym-counter 0)
(defun gensym (&optional prefix)
"Return an uninterned symbol whose name is made by appending a prefix
(defaults to \"g\") to an increasing counter."
  (let ((num (prog1 gensym-counter
               (setq gensym-counter (1+ gensym-counter)))))
    (make-symbol (format "%s%d" (or prefix "g") num))))


)code"; }}
