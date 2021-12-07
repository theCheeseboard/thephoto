import EventEmitter from "eventemitter3";

let controller;

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
        controller.emit("progressUpdated");
    }
    
    addProgress(value) {
        this.#progress += value;
        controller.emit("progressUpdated");
    }
    
    setTotal(value) {
        this.#total = value;
        controller.emit("progressUpdated");
    }
    
    setImageData(value) {
        this.#imageData = value;
    }

    imageData() {
        return this.#imageData;
    }
    
    progress() {
        return this.#progress;
    }
    
    total() {
        return this.#total;
    }

    percentComplete() {
        return this.#progress / this.#total;
    }
}

class TransferController extends EventEmitter {
    #jobs;

    constructor() {
        super();
        this.#jobs = [];
    }

    makeTransferJob() {
        let job = new TransferJob();
        this.#jobs.push(job);
        return job;
    }

    jobs() {
        // return [...this.#jobs, ...this.#jobs, ...this.#jobs, ...this.#jobs, ...this.#jobs, ...this.#jobs, ...this.#jobs, ...this.#jobs, ...this.#jobs, ...this.#jobs, ...this.#jobs, ...this.#jobs, ...this.#jobs, ...this.#jobs, ...this.#jobs];
        return this.#jobs;
    }
}

controller = new TransferController();
export default controller;