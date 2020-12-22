class TransferJob {
    #progress;
    #total;
    #imageData;
    
    constructor() {
        this.#progress = 0;
        this.#total = 0;
    }
    
    setProgress(value) {
        this.#progress = value;
    }
    
    addProgress(value) {
        this.#progress += value;
    }
    
    setTotal(value) {
        this.#total = value;
    }
    
    setImageData(value) {
        this.#imageData = value;
    }
    
    progress() {
        return this.#progress;
    }
    
    total() {
        return this.#total;
    }
}

class TransferController {
    makeTransferJob() {
        let job = new TransferJob();
        return job;
    }
}

let controller = new TransferController();
export default controller;